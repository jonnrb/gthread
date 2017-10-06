/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//platform/timer_posix.c
 * info: platform timer utilities
 */

// let's us build in C11 mode
#define _GNU_SOURCE

#include "platform/timer.h"

#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

static uint64_t g_interval = 0;
static struct itimerval g_timer;

static gthread_timer_trap_t* g_trap = NULL;
static bool g_time_slice_trap_set = false;

/**
 * ITIMER_PROF is the right timer to use here since it measures stime+utime.
 * if an active thread is doing tons of syscalls, it should be penalized for
 * the time they take.
 */
const int TIMER_TYPE = ITIMER_PROF;
const int SIGNAL_TYPE = SIGPROF;

static int set_interval_internal(uint64_t usec) {
  g_timer.it_value.tv_sec = usec / 1000000;
  g_timer.it_value.tv_usec = usec % 1000000;
  g_timer.it_interval.tv_sec = usec / 1000000;
  g_timer.it_interval.tv_usec = usec % 1000000;

  if (setitimer(TIMER_TYPE, &g_timer, NULL)) return -1;

  return 0;
}

int gthread_timer_set_interval(uint64_t usec) {
  set_interval_internal(usec);

  // read the interval back. it gets rounded up APPARENTLY
  if (getitimer(TIMER_TYPE, &g_timer)) return -1;
  g_interval = g_timer.it_value.tv_sec * 1000000 + g_timer.it_value.tv_usec;

  return 0;
}

int64_t gthread_timer_reset() {
  if (getitimer(TIMER_TYPE, &g_timer) < 0) return -1;
  uint64_t remaining =
      g_timer.it_value.tv_sec * 1000000 + g_timer.it_value.tv_usec;
  if (set_interval_internal(g_interval) < 0) return -1;
  return g_interval - remaining;
}

static void gthread_timer_alarm(int signum) {
  sigset_t sigset;
  sigaddset(&sigset, SIGNAL_TYPE);
  sigprocmask(SIG_UNBLOCK, &sigset, NULL);

  if (g_trap) g_trap(g_interval);
}

int gthread_timer_alarm_now() {
  uint64_t elapsed = gthread_timer_reset();
  if (g_trap) g_trap(elapsed);
  return 0;
}

void gthread_timer_set_trap(gthread_timer_trap_t* trap) {
  g_trap = trap;
  if (!g_time_slice_trap_set) {
    g_time_slice_trap_set = true;
    signal(SIGNAL_TYPE, gthread_timer_alarm);
  }
}
