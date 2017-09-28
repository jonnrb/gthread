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
  asm("  cmpxchgq %3, (%2)        \n"  // compare and exchange instruction
                                       // (no {smp,lock prefix,mesi} == fast)
      "  jnz compare_and_swap_fail\n"  // sets zf if successful
      "  movl $1, %0              \n"  // if successful, return 1
      "compare_and_swap_fail:     \n"
      : "=mr"(result), "+a"(expected)
      : "r"(location), "r"(new_val));
  return result;
}

#endif  // ARCH_ATOMIC_H_
