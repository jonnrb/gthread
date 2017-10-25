/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//platform/timer_test.cc
 * info: test platform timer utilities
 */

#include "platform/timer.h"

#include <assert.h>
#include <inttypes.h>
#include <sched.h>
#include <stdio.h>
#include <unistd.h>

#include "platform/clock.h"

const static int g_usec_interval = 5000;
const static int g_usec_interval_big = 20000;
static volatile int g_traps = 0;
static uint64_t g_elapsed;
static uint64_t g_last;

void trap(uint64_t elapsed) {
  ++g_traps;
  printf("elapsed from gthread_timer: %f s\n",
         (double)(g_elapsed = elapsed) / 1000000.0);
  fflush(stdout);
  int64_t now = gthread_clock_process();
  assert(now >= 0);
  printf("elapsed from process clock: %f s\n",
         (double)(now - g_last) / 1000000000.0f);
  g_last = now;
}

int main() {
  // set the trap function
  gthread_timer_set_trap(trap);

  // let a bunch of intervals run
  printf("running 10 intervals of %d µs\n", g_usec_interval);
  assert(!gthread_timer_set_interval(g_usec_interval));
  int64_t now, start = gthread_clock_process();
  assert(start >= 0);
  while (g_traps < 10)
    ;
  now = gthread_clock_process();
  assert(now >= 0);
  double d_elapsed = (double)(now - start) / (1000 * 1000 * 1000);
  double d_10traps = (double)g_usec_interval * 10 / 1000000;
  printf("elapsed for 10 intervals (process time s): %f\n",
         d_elapsed * 1000 * 1000);
  printf("elapsed for per interval (process time s): %f\n",
         d_elapsed * 1000 * 1000 / 10);
  fflush(stdout);
  assert(d_elapsed >= d_10traps);

  // clear the interval
  assert(!gthread_timer_set_interval(0));
  int cur_traps = g_traps;
  start = gthread_clock_process();
  assert(start >= 0);
  while ((now = gthread_clock_process()) - start < g_usec_interval * 2 * 1000)
    ;
  assert(now >= 0);
  assert(cur_traps == g_traps);

  // early alarm
  printf("setting interval of %d µs\n", g_usec_interval_big);
  printf("waiting for process clock to elapse %d µs\n",
         g_usec_interval_big / 2);
  assert(!gthread_timer_set_interval(g_usec_interval_big));
  start = gthread_clock_process();
  g_traps = 0;
  while (g_traps < 1) {
    now = gthread_clock_process();
    assert(now >= 0);
    if (now - start > g_usec_interval_big * 1000 / 2) {
      assert(!gthread_timer_alarm_now());
    }
  }
  assert(g_elapsed > 0 && g_elapsed < g_usec_interval_big);
}
