#include "arch/switch_to.h"

#include <iostream>

#include "platform/memory.h"

constexpr auto k_num_context_switches = 10;

const char* test_str = "hello world";

static int good = 0;

static gthread_saved_ctx_t main_ctx;
static gthread_saved_ctx_t test_func_ctx;

void test_func(void* arg) {
  char* c_arg = (char*)arg;
  std::cout << c_arg << std::endl;
  good = 1;

  for (auto i = 0; i < k_num_context_switches; ++i) {
    __asm__ __volatile__("" ::: "memory");
    gthread_switch_to(&test_func_ctx, &main_ctx);
  }

  gthread_switch_to(nullptr, &main_ctx);
  abort();
}

int main() {
  using namespace gthread;

  void* stack;
  size_t stack_size;
  gthread::allocate_stack(gthread::k_default_attr, &stack, &stack_size);

  gthread_switch_to_and_spawn(&main_ctx, stack, test_func, (void*)test_str);

  if (!good) {
    std::cout << "bad" << std::endl;
    return -1;
  }

  for (auto i = 0; i < k_num_context_switches; ++i) {
    gthread_switch_to(&main_ctx, &test_func_ctx);
  }

  return !good;
}
