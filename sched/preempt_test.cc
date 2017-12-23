#include "sched/preempt.h"

#include <iostream>

#include "platform/clock.h"

struct testy {
  testy() { std::cout << "there" << std::endl; }
  void go() { std::cout << "go" << std::endl; }
};

testy& t() {
  static testy g_testy;
  return g_testy;
}

int main() {
  gthread::enable_timer_preemption();

  using namespace std::literals;
  auto start = gthread::thread_clock::now();

  while (gthread::thread_clock::now() - start < 2s)
    ;
  t().go();
}
