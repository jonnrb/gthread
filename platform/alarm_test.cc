#include "platform/alarm.h"

#include <cassert>
#include <iostream>

#include <sched.h>

#include "platform/clock.h"

using namespace gthread;

constexpr std::chrono::microseconds k_usec_interval{5000};
constexpr std::chrono::microseconds k_usec_interval_big{20000};

static volatile int g_traps = 0;
static std::chrono::microseconds g_elapsed;
static thread_clock::time_point g_last;

using float_duration = std::chrono::duration<float>;

void trap(std::chrono::microseconds elapsed) {
  ++g_traps;
  g_elapsed = elapsed;
  std::cout << "elapsed from timer: "
            << std::chrono::duration_cast<float_duration>(elapsed).count()
            << "s" << std::endl;
  auto now = thread_clock::now();
  assert((now - g_last).count() >= 0);
  std::cout << "elapsed from process clock: "
            << std::chrono::duration_cast<float_duration>(now - g_last).count()
            << "s" << std::endl;
  g_last = now;
}

int main() {
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

  assert(elapsed >= intervals);

  // clear the interval
  alarm::clear_interval();
  int cur_traps = g_traps;
  start = thread_clock::now();
  while ((now = thread_clock::now()) - start < k_usec_interval * 2)
    ;
  assert(cur_traps == g_traps);

  // early alarm
  std::cout << "setting interval of " << k_usec_interval_big.count() << " µs "
            << std::endl;
  std::cout << "waiting for process clock to elapse "
            << (k_usec_interval_big / 2).count() << " µs" << std::endl;
  alarm::set_interval(k_usec_interval_big);
  start = thread_clock::now();
  g_traps = 0;
  while (g_traps < 1) {
    now = thread_clock::now();
    if (now - start > k_usec_interval_big / 2) {
      alarm::ring_now();
    }
  }
  assert(g_elapsed > std::chrono::microseconds{0} &&
         g_elapsed < k_usec_interval_big);
}
