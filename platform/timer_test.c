/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//platform/timer_test.c
 * info: test platform timer utilities
 */

#include <assert.h>
#include <sched.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "platform/timer.h"

const static int g_usec_interval = 5000;
const static int g_usec_interval_big = 20000;
static int g_traps = 0;
static uint64_t g_elapsed;

void trap(uint64_t elapsed) {
  ++g_traps;
  printf("elapsed: %llu\n", (g_elapsed = elapsed));
  fflush(stdout);
}

int main() {
  assert(CLOCKS_PER_SEC == 1000000);  // pretty standard

  // set the trap function
  gthread_timer_set_trap(trap);

  // let a bunch of intervals run
  assert(!gthread_timer_set_interval(g_usec_interval));
  clock_t start = clock();
  while (g_traps < 20)
    ;
  double d_elapsed = (double)(clock() - start) / CLOCKS_PER_SEC;
  double d_20traps = (double)g_usec_interval * 20 / 1000000;
  printf("elapsed time for 20 intervals: %f\n", d_elapsed);
  fflush(stdout);
  assert(d_elapsed >= d_20traps);

  // clear the interval
  assert(!gthread_timer_set_interval(0));
  int cur_traps = g_traps;
  start = clock();
  while (clock() - start < g_usec_interval * 2)
    ;
  assert(cur_traps == g_traps);

  // early alarm
  assert(!gthread_timer_set_interval(g_usec_interval_big));
  start = clock();
  g_traps = 0;
  while (g_traps < 1) {
    if (clock() - start > g_usec_interval_big / 2) {
      assert(!gthread_timer_alarm_now());
    }
  }
  assert(g_elapsed > 0 && g_elapsed < g_usec_interval_big);
}
