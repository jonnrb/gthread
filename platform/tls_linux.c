/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//platform/tls_linux.c
 * info: implement thread local storage (ELF spec) on linux piggybacking glibc
 */

#define _GNU_SOURCE

#include "platform/tls.h"

#include <asm/prctl.h>
#include <link.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "util/compiler.h"

// has to be the same as glibc right now so we can use their internal functions
typedef union dtv {
  size_t num_modules;
  struct {
    void* v;
    bool is_static;  // for now, don't care about this
  } pointer;
} dtv_t;

// this is how glibc detects unallocated slots for dynamic loading
#define TLS_DTV_UNALLOCATED ((void*)-1l)

// should be enough slots i hope
#define k_num_slots 128

static inline int get_dtv(dtv_t** dtv) {
  return syscall(SYS_arch_prctl, ARCH_GET_FS, dtv);
}

static inline int set_dtv(dtv_t* dtv) {
  return syscall(SYS_arch_prctl, ARCH_SET_FS, dtv);
}

/**
 * |data| must not be NULL. iterates over all modules and assumes they're
 * static because otherwise bad things. adds all the sizes into
 * `space_before_thread_pointer` so the caller of `gthread_tls_allocate()`
 * knows how much extra space to add to the tcb for tls data.
 */
static int dl_iterate_phdr_exec_size_cb(struct dl_phdr_info* info, size_t size,
                                        void* data) {
  size_t* acc = (size_t*)data;
  const ElfW(Phdr)* phdr_base = info->dlpi_phdr;
  const int phdr_num = info->dlpi_phnum;

  for (const ElfW(Phdr)* phdr = phdr_base + phdr_num - 1; phdr >= phdr_base;
       --phdr) {
    if (phdr->p_type == PT_TLS) {
      // `p_memsz` determines how much space should be reserved for the
      // segment. must be rounded up to alignment boundary.
      acc[0] += phdr->p_memsz + ((-phdr->p_memsz) & (phdr->p_align - 1));
      if (phdr->p_align > acc[1]) acc[1] = phdr->p_align;
      break;
    }
  }

  return 0;
}

gthread_tls_t gthread_tls_allocate(size_t* tls_image_reserve,
                                   size_t* tls_align) {
  dtv_t* dtv = calloc(k_num_slots + 2, sizeof(dtv_t));

  dtv[0].num_modules = k_num_slots;

  for (int i = 0; i < k_num_slots; ++i) {
    dtv[i + 2].pointer.v = TLS_DTV_UNALLOCATED;
    dtv[i + 2].pointer.is_static = 0;
  }

  if (tls_image_reserve != NULL || tls_align != NULL) {
    size_t acc[2] = {0};
    dl_iterate_phdr(dl_iterate_phdr_exec_size_cb, acc);
    if (tls_image_reserve != NULL) *tls_image_reserve = acc[0];
    if (tls_align != NULL) *tls_align = acc[1];
  }

  return (gthread_tls_t)dtv;
}

void gthread_tls_free(gthread_tls_t tls) {
  if (tls == NULL) return;

  dtv_t* dtv = (dtv_t*)tls;
  for (int i = 0; i < k_num_slots; ++i) {
    if (!dtv[i + 2].pointer.is_static &&
        dtv[i + 2].pointer.v != TLS_DTV_UNALLOCATED) {
      free(dtv[i + 2].pointer.v);
    }
  }

  free(dtv);
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
        images[module - 1].data = info->dlpi_tls_data;
        images[module - 1].image_size = phdr->p_filesz;
        images[module - 1].mem_offset = (-phdr->p_memsz) & (phdr->p_align - 1);
        images[module - 1].reserve = phdr->p_memsz;
      } else {
        // must have a module id (index starts at 1)
        return -1;
      }
      break;
    }
  }

  return 0;
}

int gthread_tls_initialize_image(gthread_tls_t tls) {
  dtv_t* dtv = (dtv_t*)tls;
  tls_image_t images[k_num_slots];

  if (dtv[1].pointer.v == NULL) return -1;

  for (int i = 0; i < k_num_slots; ++i) {
    images[i].data = NULL;
  }

  if (dl_iterate_phdr(dl_iterate_phdr_init_cb, images)) return -1;

  char* base = (char*)dtv[1].pointer.v;
  for (int i = 0; i < k_num_slots; ++i) {
    if (images[i].data == NULL) break;

    size_t uninitialized = images[i].reserve - images[i].image_size;

    // this data ordering *seems* to work and models gnu and musl libcs
    base -= images[i].image_size;
    base -= images[i].mem_offset;
    memcpy(base, images[i].data, images[i].image_size);
    base -= uninitialized;

    dtv[i + 2].pointer.is_static = 1;
    dtv[i + 2].pointer.v = base;
  }

  return 0;
}

gthread_tls_t gthread_tls_current() {
  dtv_t* dtv = NULL;
  get_dtv(&dtv);
  return dtv - 1;
}

void gthread_tls_set_thread(gthread_tls_t tls, void* thread) {
  dtv_t* dtv = (dtv_t*)tls;
  dtv[1].pointer.v = thread;
}

void* gthread_tls_get_thread(gthread_tls_t tls) {
  dtv_t* dtv = (dtv_t*)tls;
  return dtv[1].pointer.v;
}

void gthread_tls_use(gthread_tls_t tls) {
  dtv_t* dtv = (dtv_t*)tls;
  set_dtv(&dtv[1]);
}
