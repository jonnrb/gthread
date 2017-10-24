/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//platform/memory_macos.cc
 * info: implements macOS specific functionality
 */

#include "platform/memory.h"

#include <errno.h>
#include <mach/mach_init.h>
#include <mach/mach_vm.h>
#include <mach/vm_page_size.h>

#include "arch/bit_twiddle.h"
#include "gthread.h"
#include "util/compiler.h"

namespace gthread {

static constexpr mach_vm_address_t k_stack_hint = 0xB0000000;

const size_t k_stack_min = 0x2000;

// use mach_vm_map for xnu. based on what apple libc pthreads.
int allocate_stack(const attr& a, void** stack, size_t* total_stack_size) {
  // user provided stack space. no guard pages setup in this case.
  if (a.stack.addr != NULL) {
    if (branch_unexpected(((uintptr_t)a.stack.addr % vm_page_size) != 0)) {
      return -1;
    }
    *stack = a.stack.addr;
    return 0;
  }

  size_t t;
  if (total_stack_size == NULL) total_stack_size = &t;
  *total_stack_size = a.stack.size + a.stack.guardsize;

  // tags the region as a stack. could also be done with mmap() but would
  // require different flags than linux and xnu doesnt have MAP_GROWSDOWN so why
  // bother.
  mach_vm_address_t stackaddr = k_stack_hint;  // actual stack gets put here
  kern_return_t kr = mach_vm_map(
      mach_task_self(), &stackaddr, *total_stack_size,
      right_mask(*total_stack_size),
      VM_MAKE_TAG(VM_MEMORY_STACK) | VM_FLAGS_ANYWHERE, MEMORY_OBJECT_NULL, 0,
      FALSE, VM_PROT_DEFAULT, VM_PROT_ALL, VM_INHERIT_DEFAULT);

  // fallback.
  if (kr != KERN_SUCCESS) {
    kr = mach_vm_allocate(mach_task_self(), &stackaddr, *total_stack_size,
                          VM_MAKE_TAG(VM_MEMORY_STACK) | VM_FLAGS_ANYWHERE);
  }
  if (kr != KERN_SUCCESS) {
    return EAGAIN;
  }

  // the guard page is at the lowest address. apply protection to segv there.
  if (a.stack.guardsize != 0) {
    size_t guardsize = a.stack.guardsize != static_cast<size_t>(-1)
                           ? a.stack.guardsize
                           : k_stack_min;
    kr = mach_vm_protect(mach_task_self(), stackaddr, guardsize, FALSE,
                         VM_PROT_NONE);
  }
  *stack = (void*)(stackaddr + *total_stack_size);
  return 0;
}

int free_stack(void* stack_base, size_t total_stack_size) {
  if (mach_vm_deallocate(mach_task_self(), (mach_vm_address_t)stack_base,
                         total_stack_size)) {
    return EAGAIN;
  }
  return 0;
}

}  // namespace gthread
