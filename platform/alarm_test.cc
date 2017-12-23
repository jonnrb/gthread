#include "platform/alarm.h"

#include <cassert>
#include <iostream>

#include <sched.h>

#include "gtest/gtest.h"
#include "platform/clock.h"

using namespace gthread;

constexpr std::chrono::microseconds k_usec_interval{5000};

static volatile int g_traps = 0;
static thread_clock::time_point g_last;

using float_duration = std::chrono::duration<float>;

void trap() {
  ++g_traps;
  auto now = thread_clock::now();
  assert((now - g_last).count() >= 0);
  std::cout << "elapsed from process clock: "
            << std::chrono::duration_cast<float_duration>(now - g_last).count()
            << "s" << std::endl;
  g_last = now;
}

TEST(gthread_alarm, several_intervals) {
  // set the trap function
  alarm::set_trap(trap);

  // let a bunch of intervals run
  std::cout << "running 10 intervals of " << k_usec_interval.count() << " µs"
            << std::endl;
  alarm::set_interval(k_usec_interval);
  auto start = thread_clock::now();
  while (g_traps < 10)
    ;
  auto now = thread_clock::now();

  auto elapsed = std::chrono::duration_cast<float_duration>(now - start);
  auto intervals =
      std::chrono::duration_cast<float_duration>(k_usec_interval * 10);

  std::cout << "elapsed for 10 intervals (process time s): " << elapsed.count()
            << std::endl;
  std::cout << "elapsed for per interval (process time s): "
            << intervals.count() << std::endl;

  EXPECT_GE(elapsed, intervals);

  // clear the interval
  alarm::clear_interval();
  int cur_traps = g_traps;
  start = thread_clock::now();
  while ((now = thread_clock::now()) - start < k_usec_interval * 2)
    ;
  EXPECT_EQ(cur_traps, g_traps);
}
