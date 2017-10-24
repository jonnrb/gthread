/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//platform/free_stack_test.cc
 * info: tests allocating a stack and then freeing it
 */

#include <assert.h>
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

  printf("ret:   %d\n", gthread::allocate_stack(t_attr, &t_attr.stack.addr,
                                                &total_stack_size));
  printf("size:  0x%zu\n", total_stack_size);
  printf("stack: %p\n", t_attr.stack.addr);
  printf("base:  %p\n", (char*)t_attr.stack.addr - total_stack_size);

  // can write to stack now
  ((uint64_t*)t_attr.stack.addr)[-1] = 4;

  assert(!gthread::free_stack((char*)t_attr.stack.addr - total_stack_size,
                              total_stack_size));

  // now we can't
  ok = 1;
  ((uint64_t*)t_attr.stack.addr)[-1] = 4;

  return 1;
}
