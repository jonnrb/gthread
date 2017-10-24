/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//arch/switch_to_test.cc
 * info: tests switching between and spawning new contexts
 */

#define _XOPEN_SOURCE

#include "arch/switch_to.h"

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <ucontext.h>

#include "platform/clock.h"
#include "platform/memory.h"

constexpr uint64_t k_num_context_switches = 1000 * 1000;

const char* test_str = "hello world";

static int good = 0;
static int also_good = 0;

static gthread_saved_ctx_t main_ctx;
static gthread_saved_ctx_t test_func_ctx;

static ucontext_t main_uctx;
static ucontext_t utest_func_uctx;

void test_func(void* arg) {
  char* c_arg = (char*)arg;
  printf("%s\n", c_arg);
  good = 1;

  for (uint64_t i = 0; i < k_num_context_switches; ++i) {
    __asm__ __volatile__("" ::: "memory");
    gthread_switch_to(&test_func_ctx, &main_ctx);
  }

  gthread_switch_to(nullptr, &main_ctx);
  abort();
}

void utest_func() {
  also_good = 1;

  for (uint64_t i = 0; i < k_num_context_switches; ++i) {
    __asm__ __volatile__("" ::: "memory");
    swapcontext(&utest_func_uctx, &main_uctx);
  }
}

int main() {
  void* stack;
  size_t stack_size;
  gthread::allocate_stack(k_default_attr, &stack, &stack_size);

  gthread_switch_to_and_spawn(&main_ctx, stack, test_func, (void*)test_str);

  if (!good) {
    printf("bad\n");
    return -1;
  }

  uint64_t start = gthread_clock_process();
  for (uint64_t i = 0; i < k_num_context_switches; ++i) {
    gthread_switch_to(&main_ctx, &test_func_ctx);
  }
  uint64_t end = gthread_clock_process();

  printf("gthread_switch_to() microbenchmark:\n");
  printf("  %" PRIu64 " context switches in %.3f microseconds\n",
         k_num_context_switches * 2, (double)(end - start) / 1000);
  printf("  %.0f context switches / microsecond\n",
         k_num_context_switches * 2 / ((double)(end - start) / 1000));

  assert(!getcontext(&utest_func_uctx));
  utest_func_uctx.uc_stack.ss_sp = &((char*)stack)[-stack_size];
  utest_func_uctx.uc_stack.ss_size = stack_size;
  utest_func_uctx.uc_link = &main_uctx;
  makecontext(&utest_func_uctx, utest_func, 0);

  start = gthread_clock_process();
  for (uint64_t i = 0; i < k_num_context_switches; ++i) {
    swapcontext(&main_uctx, &utest_func_uctx);
  }
  end = gthread_clock_process();

  printf("swapcontext() microbenchmark:\n");
  printf("  %" PRIu64 " context switches in %.3f microseconds\n",
         k_num_context_switches * 2, (double)(end - start) / 1000);
  printf("  %.0f context switches / microsecond\n",
         k_num_context_switches * 2 / ((double)(end - start) / 1000));

  return !good;
}
