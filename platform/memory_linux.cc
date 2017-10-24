/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//platform/memory_linux.c
 * info: implements Linux specific functionality
 */

#include "platform/memory.h"

#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>

#include "arch/bit_twiddle.h"
#include "gthread.h"

const size_t GTHREAD_STACK_MIN = 0x2000;

// same as nptl
static constexpr size_t k_stack_default = 2 << 20;

static inline size_t page_size() {
  static size_t k = 0;
#ifdef _SC_PAGESIZE
  if (k == 0) k = (size_t)sysconf(_SC_PAGESIZE);
#elif defined(PAGE_SIZE)
  if (k == 0) k = (size_t)sysconf(PAGE_SIZE);
#else
#error "no PAGESIZE macro for this system! is this a POSIX system?"
#endif
  return k;
}

// use mmap() to allocate a stack and mprotect() for the guard page
int gthread_allocate_stack(gthread_attr_t *attrs, void **stack,
                           size_t *total_stack_size) {
  static gthread_attr_t defaults = {
      {GTHREAD_STACK_MIN, k_stack_default, nullptr}};
  attrs = attrs ?: &defaults;

  // user provided stack space. no guard pages setup in this case.
  if (attrs->stack.addr != nullptr) {
    if (branch_unexpected(((uintptr_t)attrs->stack.addr % page_size()) != 0)) {
      return -1;
    }
    *stack = attrs->stack.addr;
    return 0;
  }

  size_t t;
  if (total_stack_size == nullptr) total_stack_size = &t;
  *total_stack_size = attrs->stack.size + attrs->stack.guardsize;

  // mmap the region as a stack using MAP_GROWSDOWN magic
  void *stackaddr = mmap(nullptr, *total_stack_size, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS | MAP_GROWSDOWN, -1, 0);

  if (stackaddr == nullptr) {
    return -1;
  }

  // the guard page is at the lowest address. apply protection to segv there.
  if (attrs->stack.guardsize) {
    if (mprotect(stackaddr, attrs->stack.guardsize, PROT_NONE)) {
      return -1;
    }
  }
  *stack = (void *)((char *)stackaddr + *total_stack_size);
  return 0;
}

int gthread_free_stack(void *stack_base, size_t total_stack_size) {
  return munmap((char *)stack_base, total_stack_size);
}

#ifdef MACOS_STACK_TEST
#endif  // MACOS_STACK_TEST
