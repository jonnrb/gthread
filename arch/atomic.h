/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//arch/atomic.h
 * info: atomic operation inlines
 */

#ifndef ARCH_ATOMIC_H_
#define ARCH_ATOMIC_H_

#include <stdint.h>

static inline int compare_and_swap(uint64_t* location, uint64_t expected,
                                   uint64_t new_val) {
  int result = 0;
  __asm__ __volatile__(
      "  mov %2, %%rax    \n"  // |expected| goes in rax
      "  cmpxchgq %3, (%1)\n"  // compare and exchange instruction
                               // (no {smp,lock prefix,mesi} == fast)
      "  jnz 1f           \n"  // sets zf if successful
      "  movl $1, %0      \n"  // if successful, return 1
      "1:                 \n"
      : "=m"(result)
      : "D"(location), "S"(expected), "d"(new_val)
      : "rax");
  return result;
}

#endif  // ARCH_ATOMIC_H_
