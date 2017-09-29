/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//platform/free_stack_test.c
 * info: tests allocating a stack and then freeing it
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "platform/memory.h"

int main() {
  gthread_attr_t t_attr;
  t_attr.stack.size = GTHREAD_STACK_MIN;
  t_attr.stack.guardsize = GTHREAD_STACK_MIN;
  t_attr.stack.addr = NULL;
  size_t total_stack_size;

  printf("ret:   %d\n", gthread_allocate_stack(&t_attr, &t_attr.stack.addr,
                                               &total_stack_size));
  printf("size:  0x%zu\n", total_stack_size);
  printf("stack: %p\n", t_attr.stack.addr);
  printf("base:  %p\n", t_attr.stack.addr - total_stack_size);

  assert(!gthread_free_stack(t_attr.stack.addr - total_stack_size,
                             total_stack_size));

  return 0;
}
