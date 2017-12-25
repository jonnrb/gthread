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
#include "util/log.h"

extern "C" {
void __ctype_init();
}

namespace gthread {
namespace {
// has to be the same as glibc right now so we can use their internal functions
union dtv {
  struct {
    size_t num_modules;
    void* base;
  } head;
  struct {
    void* v;
    bool is_static;  // for now, don't care about this
  } pointer;
};

struct tcbhead {
  void* self;
  dtv* thread_vector;  // maintain compatability with glibc. for our purposes,
                       // the `dtv` will be immediately after the `tcbhead`.
  void* thread;

  // `__ctype_init()` must be called upon entry to a new tls context
  char did_ctype_init;

  char padding_a[7];

  // more glibc required MAGIC
  void* sysinfo;
  void* stack_guard;
  void* pointer_guard;

  char padding_b[1632];  // yes glibc's pthread_t is horrifically large

  size_t glibc_stack_size;
};

// this is how glibc detects unallocated slots for dynamic loading
#define TLS_DTV_UNALLOCATED ((void*)-1l)

// should be enough slots i hope. it is dangerous if the runtime calls a
// `realloc()` on the `dtv` right now, but i can't forsee more than 16
// modules being loaded at a time.
constexpr int k_num_slots = 16;

int get_tcb(tcbhead** tcb) { return syscall(SYS_arch_prctl, ARCH_GET_FS, tcb); }

int set_tcb(tcbhead* tcb) { return syscall(SYS_arch_prctl, ARCH_SET_FS, tcb); }

struct tls_image {
  void* data;
  size_t image_size;
  size_t mem_offset;
  size_t reserve;  // the tls image contains initialized data. the tls segments
                   // often need uninitialized space reserved.
};

/**
 * finds the ELF TLS header for each loaded module.
 *
 * |data| is a `tls_image` array big enough for all the tls modules.
 */
int dl_iterate_phdr_init_cb(struct dl_phdr_info* info, size_t size,
                            void* data) {
  tls_image* images = (tls_image*)data;
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
tcbhead magic_pointers;

// needed to get consistent values after the context has been switched
tls_image* find_tls_images() {
  static bool has = false;
  static tls_image images[k_num_slots];

  if (branch_expected(has)) return images;

  for (int i = 0; i < k_num_slots; ++i) {
    images[i].data = nullptr;
  }

  // discover tls images in the binary
  if (dl_iterate_phdr(dl_iterate_phdr_init_cb, images)) return NULL;

  tcbhead* og_tcb_head;
  get_tcb(&og_tcb_head);
  magic_pointers.sysinfo = og_tcb_head->sysinfo;
  magic_pointers.stack_guard = og_tcb_head->stack_guard;
  magic_pointers.pointer_guard = og_tcb_head->pointer_guard;
  magic_pointers.glibc_stack_size = og_tcb_head->glibc_stack_size;

  has = true;

  return images;
}
}  // namespace

tls::tls() {
  tcbhead* tcb = (tcbhead*)this;
  tcb->self = tcb;

  dtv* thread_vector = new dtv[k_num_slots + 2];
  tcb->thread_vector = thread_vector;
  thread_vector[0].head.num_modules = k_num_slots;
  thread_vector[0].head.base = (char*)this - prefix_bytes();
  thread_vector[1].pointer.v = (void*)tcb;
  thread_vector[1].pointer.is_static = false;
  for (int i = 0; i < k_num_slots; ++i) {
    thread_vector[i + 2].pointer.v = TLS_DTV_UNALLOCATED;
    thread_vector[i + 2].pointer.is_static = false;
  }

  tls_image* images = find_tls_images();
  char* image_base = (char*)this;
  int module = 0;
  for (int i = 0; i < k_num_slots; ++i) {
    if (images[i].data == NULL) continue;
    ++module;

    // this data ordering *seems* to work and models gnu and musl libcs
    image_base -= images[i].reserve;
    memcpy(image_base + images[i].mem_offset,
           (char*)images[i].data + images[i].mem_offset, images[i].image_size);

    thread_vector[module + 1].pointer.is_static = true;
    thread_vector[module + 1].pointer.v = image_base;
  }

  tcb->sysinfo = magic_pointers.sysinfo;
  tcb->stack_guard = magic_pointers.stack_guard;
  tcb->pointer_guard = magic_pointers.pointer_guard;
  tcb->glibc_stack_size = magic_pointers.glibc_stack_size;
  tcb->did_ctype_init = 0;
}

tls::~tls() {
  dtv* thread_vector = ((tcbhead*)this)->thread_vector;
  for (int i = 0; i < k_num_slots; ++i) {
    if (!thread_vector[i + 2].pointer.is_static &&
        thread_vector[i + 2].pointer.v != TLS_DTV_UNALLOCATED) {
      free(thread_vector[i + 2].pointer.v);
    }
  }

  delete[] thread_vector;
}

size_t tls::prefix_bytes() {
  tls_image* images = find_tls_images();
  size_t static_images_size = 0;
  for (int i = 0; i < k_num_slots; ++i) {
    if (images[i].data == nullptr) break;
    static_images_size += images[i].reserve;
  }
  return static_images_size;
}

size_t tls::postfix_bytes() { return sizeof(tcbhead); }

void tls::reset() {
  // free dynamically allocated modules
  dtv* thread_vector = ((tcbhead*)this)->thread_vector;
  for (int i = 0; i < k_num_slots; ++i) {
    if (thread_vector[i + 2].pointer.v != TLS_DTV_UNALLOCATED &&
        !thread_vector[i + 2].pointer.is_static) {
      free(thread_vector[i + 2].pointer.v);
    }
    thread_vector[i + 2].pointer.v = TLS_DTV_UNALLOCATED;
    thread_vector[i + 2].pointer.is_static = false;
  }

  tls_image* images = find_tls_images();

  // zero-initialize data not in the image
  char* old_base = (char*)thread_vector[0].head.base;
  memset(old_base, '\0', (char*)this - old_base);

  char* image_base = (char*)this;
  int module = 0;
  for (int i = 0; image_base > old_base && i < k_num_slots; ++i) {
    if (images[i].data == nullptr) continue;
    ++module;

    // this data ordering *seems* to work and models gnu and musl libcs
    image_base -= images[i].reserve;
    if (branch_unexpected(image_base < old_base)) {
      gthread_log_fatal("tls images inconsistent");
    }
    memcpy(image_base, images[i].data, images[i].image_size);

    thread_vector[module + 1].pointer.is_static = true;
    thread_vector[module + 1].pointer.v = image_base;
  }

  ((tcbhead*)this)->did_ctype_init = 0;
}

void tls::set_thread(void* thread) { ((tcbhead*)this)->thread = thread; }

void* tls::get_thread() { return ((tcbhead*)this)->thread; }

tls* tls::current() {
  tcbhead* cur = nullptr;
  get_tcb(&cur);
  return (tls*)cur;
}

void tls::use() {
  set_tcb((tcbhead*)this);
  if (branch_unexpected(!((tcbhead*)this)->did_ctype_init)) {
    __ctype_init();
    ((tcbhead*)this)->did_ctype_init = 1;
  }
}

}  // namespace gthread
