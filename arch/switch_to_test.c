/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//arch/switch_to_test.c
 * info: tests switching between and spawning new contexts
 */

#include <stdio.h>

#include "platform/memory.h"
#include "switch_to.h"

const char* test_str = "hello world";

static int good = 0;

void test_func(void* arg) {
  char* c_arg = (char*)arg;
  printf("%s\n", c_arg);
  good = 1;
}

int main() {
  void* stack;
  gthread_allocate_stack(NULL, &stack, NULL);

  gthread_saved_ctx_t self;
  gthread_switch_to_and_spawn(&self, &self, stack, test_func, (void*)test_str);

  return !good;
}
