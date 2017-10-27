#include "platform/memory.h"

#include <cassert>
#include <iostream>

#include <signal.h>

volatile int ok = 0;

void segv_handler(int s) {
  std::cout << "segfault here (expected=" << ok << ")" << std::endl;
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

  std::cout << "ret:   "
            << gthread::allocate_stack(t_attr, &t_attr.stack.addr,
                                       &total_stack_size)
            << std::endl;
  std::cout << "size:  " << total_stack_size << std::endl;
  std::cout << "stack: " << t_attr.stack.addr << std::endl;
  std::cout << "base:  " << (void*)((char*)t_attr.stack.addr - total_stack_size)
            << std::endl;

  // can write to stack now
  ((uint64_t*)t_attr.stack.addr)[-1] = 4;

  assert(!gthread::free_stack((char*)t_attr.stack.addr - total_stack_size,
                              total_stack_size));

  // now we can't
  ok = 1;
  ((uint64_t*)t_attr.stack.addr)[-1] = 4;

  return 1;
}
