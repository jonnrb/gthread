/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//arch/atomic_test.c
 * info: test atomic operation inlines
 */

#include "arch/atomic.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>

void test_compare_and_swap() {
  uint64_t val = 10;

  assert(!gthread_cas(&val, 5, 0));
  assert(val == 10);

  assert(gthread_cas(&val, 10, 0));
  printf("val: %" PRIu64 "\n", val);
  assert(val == 0);
}

int main() {
  test_compare_and_swap();
  return 0;
}
