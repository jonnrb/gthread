/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//platform/allocate_stack_test.cc
 * info: tests stack allocation by allocating a stack and writing to it
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "platform/memory.h"

volatile int ok = 0;

void segv_handler(int s) {
  printf("segfault here (expected=%d)\n", ok);
  exit(ok == 0);
}

int main() {
  signal(SIGSEGV, segv_handler);
  signal(SIGBUS, segv_handler);

  gthread::attr t_attr;
  t_attr.stack.size = gthread::k_stack_min;
  t_attr.stack.guardsize = gthread::k_stack_min;
  t_attr.stack.addr = NULL;
  size_t total_stack_size;

  printf("%d ", gthread::allocate_stack(t_attr, &t_attr.stack.addr,
                                        &total_stack_size));
  printf("%p\n\n", t_attr.stack.addr);

  for (uint64_t* i = (uint64_t*)t_attr.stack.addr - 1;; --i) {
    if ((char*)t_attr.stack.addr - (char*)i > t_attr.stack.size) {
      printf("next should segfault\n");
      printf("writing to %#tx\n", (char*)t_attr.stack.addr - (char*)i);
      ok = 1;
    }
    *i = (uint64_t)i;
  }
  // should have segfaulted. will exit with failure.
  segv_handler(0);
}
