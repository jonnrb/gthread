/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//platform/tls_linux.c
 * info: implement thread local storage (ELF spec) on linux piggybacking glibc
 */

#include "platform/tls.h"

#include <asm/prctl.h>
#include <link.h>
#include <locale.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "util/compiler.h"

// has to be the same as glibc right now so we can use their internal functions
typedef union dtv {
  struct {
    size_t num_modules;
    void* base;
  } head;
  struct {
    void* v;
    bool is_static;  // for now, don't care about this
  } pointer;
} dtv_t;

typedef struct gthread_tls {
  void* self;
  dtv_t* dtv;  // maintain compatability with glibc. for our purposes, the
               // `dtv_t` will be immediately after the `tcbhead_t`.
  void* thread;

  char padding_a[8];
  // more glibc required MAGIC
  void* sysinfo;
  void* stack_guard;
  void* pointer_guard;
} tcbhead_t;

// this is how glibc detects unallocated slots for dynamic loading
#define TLS_DTV_UNALLOCATED ((void*)-1l)

// should be enough slots i hope. it is dangerous if the runtime calls a
// `realloc()` on the `dtv_t` right now, but i can't forsee more than 16
// modules being loaded at a time.
#define k_num_slots 16

static inline int get_thread_vector(tcbhead_t** tcbhead) {
  return syscall(SYS_arch_prctl, ARCH_GET_FS, tcbhead);
}

static inline int set_thread_vector(tcbhead_t* tcbhead) {
  return syscall(SYS_arch_prctl, ARCH_SET_FS, tcbhead);
}

typedef struct {
  void* data;
  size_t image_size;
  size_t mem_offset;
  size_t reserve;  // the tls image contains initialized data. the tls segments
                   // often need uninitialized space reserved.
} tls_image_t;

/**
 * finds the ELF TLS header for each loaded module.
 *
 * |data| is a `tls_image_t` array big enough for all the tls modules.
 */
static int dl_iterate_phdr_init_cb(struct dl_phdr_info* info, size_t size,
                                   void* data) {
  tls_image_t* images = (tls_image_t*)data;
  const ElfW(Phdr)* phdr_base = info->dlpi_phdr;
  const int phdr_num = info->dlpi_phnum;

  // internet wisdom is that the tls module is usually last. gnu does this the
  // same way.
  for (const ElfW(Phdr)* phdr = phdr_base + phdr_num - 1; phdr >= phdr_base;
       --phdr) {
    if (phdr->p_type == PT_TLS) {
      size_t module = info->dlpi_tls_modid;
      if (branch_expected(module > 0)) {
        images[module - 1].data = (char*)info->dlpi_addr + phdr->p_vaddr;
        images[module - 1].image_size = phdr->p_filesz;
        images[module - 1].mem_offset = (-phdr->p_memsz) & (phdr->p_align - 1);
        images[module - 1].reserve =
            images[module - 1].mem_offset + phdr->p_memsz;
      } else {
        // must have a module id (index starts at 1)
        return -1;
      }
      break;
    }
  }

  return 0;
}

// initialized in `find_tls_images()`
static tcbhead_t magic_pointers;

// needed to get consistent values after the context has been switched
static tls_image_t* find_tls_images() {
  static bool has = false;
  static tls_image_t images[k_num_slots];

  if (branch_expected(has)) return images;

  for (int i = 0; i < k_num_slots; ++i) {
    images[i].data = NULL;
  }

  // discover tls images in the binary
  if (dl_iterate_phdr(dl_iterate_phdr_init_cb, images)) return NULL;

  tcbhead_t* og_tcb_head;
  get_thread_vector(&og_tcb_head);
  magic_pointers.sysinfo = og_tcb_head->sysinfo;
  magic_pointers.stack_guard = og_tcb_head->stack_guard;
  magic_pointers.pointer_guard = og_tcb_head->pointer_guard;

  has = true;

  return images;
}

gthread_tls_t gthread_tls_allocate() {
  tls_image_t* images = find_tls_images();

  size_t alloc_size = 0;
  for (int i = 0; i < k_num_slots; ++i) {
    if (images[i].data == NULL) break;
    alloc_size += images[i].reserve;
  }
  size_t tcb_offset = alloc_size;  // save the offset for the `tcbhead_t`
  alloc_size += sizeof(tcbhead_t) + (k_num_slots + 2) * sizeof(dtv_t);

  char* alloc_base = (char*)calloc(alloc_size, 1);
  tcbhead_t* tcbhead = (tcbhead_t*)(alloc_base + tcb_offset);
  tcbhead->self = tcbhead;

  dtv_t* dtv = (dtv_t*)(tcbhead + 1);
  tcbhead->dtv = dtv;
  dtv[0].head.num_modules = k_num_slots;
  dtv[0].head.base = alloc_base;
  dtv[1].pointer.v = (void*)tcbhead;
  dtv[1].pointer.is_static = false;
  for (int i = 0; i < k_num_slots; ++i) {
    dtv[i + 2].pointer.v = TLS_DTV_UNALLOCATED;
    dtv[i + 2].pointer.is_static = false;
  }

  char* image_base = (char*)tcbhead;
  int module = 0;
  for (int i = 0; i < k_num_slots; ++i) {
    if (images[i].data == NULL) continue;
    ++module;

    // this data ordering *seems* to work and models gnu and musl libcs
    image_base -= images[i].reserve;
    memcpy((char*)image_base + images[i].mem_offset,
           (char*)images[i].data + images[i].mem_offset, images[i].image_size);

    dtv[module + 1].pointer.is_static = true;
    dtv[module + 1].pointer.v = image_base;
  }

  tcbhead->sysinfo = magic_pointers.sysinfo;
  tcbhead->stack_guard = magic_pointers.stack_guard;
  tcbhead->pointer_guard = magic_pointers.pointer_guard;

  return tcbhead;
}

int gthread_tls_reset(gthread_tls_t tls) {
  // free dynamically allocated modules
  dtv_t* dtv = tls->dtv;
  for (int i = 0; i < k_num_slots; ++i) {
    if (dtv[i + 2].pointer.v != TLS_DTV_UNALLOCATED &&
        !dtv[i + 2].pointer.is_static) {
      free(dtv[i + 2].pointer.v);
    }
    dtv[i + 2].pointer.v = TLS_DTV_UNALLOCATED;
    dtv[i + 2].pointer.is_static = false;
  }

  tls_image_t* images = find_tls_images();

  size_t total_size = 0;
  for (int i = 0; i < k_num_slots; ++i) {
    if (images[i].data == NULL) break;
    total_size += images[i].reserve;
  }

  // zero-initialize data not in the image
  char* old_base = (char*)dtv[0].head.base;
  memset(old_base, '\0', (char*)tls - old_base);

  char* image_base = (char*)tls;
  int module = 0;
  for (int i = 0; image_base > old_base && i < k_num_slots; ++i) {
    if (images[i].data == NULL) continue;
    ++module;

    // this data ordering *seems* to work and models gnu and musl libcs
    image_base -= images[i].reserve;
    if (image_base < old_base) return -1;
    memcpy(image_base, images[i].data, images[i].image_size);

    dtv[module + 1].pointer.is_static = true;
    dtv[module + 1].pointer.v = image_base;
  }

  return 0;
}

void gthread_tls_free(gthread_tls_t tls) {
  if (tls == NULL) return;

  dtv_t* dtv = tls->dtv;
  for (int i = 0; i < k_num_slots; ++i) {
    if (!dtv[i + 2].pointer.is_static &&
        dtv[i + 2].pointer.v != TLS_DTV_UNALLOCATED) {
      free(dtv[i + 2].pointer.v);
    }
  }

  free(tls->dtv[0].head.base);
}

void gthread_tls_set_thread(gthread_tls_t tls, void* thread) {
  tls->thread = thread;
}

void* gthread_tls_get_thread(gthread_tls_t tls) { return tls->thread; }

gthread_tls_t gthread_tls_current() {
  tcbhead_t* tcbhead = NULL;
  get_thread_vector(&tcbhead);
  return tcbhead;
}

void gthread_tls_use(gthread_tls_t tls) { set_thread_vector(tls); }
