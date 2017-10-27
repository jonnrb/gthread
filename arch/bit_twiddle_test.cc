#include "arch/bit_twiddle.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>

void check_right_mask(uint64_t number, uint64_t masked) {
  uint64_t actual_masked = right_mask(number);
  printf("number:   %" PRIx64 "\nexpected: %" PRIx64 "\nactual:   %" PRIx64
         "\n\n",
         number, masked, actual_masked);
  assert(masked == actual_masked);
}

int main() {
  check_right_mask(0x4F00, 0x7FFF);
  check_right_mask(0x8000000000000000, 0xFFFFFFFFFFFFFFFF);
  check_right_mask(0x6000000000000000, 0x7FFFFFFFFFFFFFFF);

  return 0;
}
