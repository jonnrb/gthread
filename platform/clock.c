#define _POSIX_C_SOURCE 199309L

#include "platform/clock.h"

#include <stdint.h>
#include <time.h>

#include "util/compiler.h"

static inline int64_t posix_time_from_type(clockid_t c) {
  struct timespec t;
  if (branch_unexpected(clock_gettime(c, &t))) {
    return -1;
  }
  return t.tv_sec * 1000 * 1000 * 1000 + t.tv_nsec;
}

static inline int64_t posix_time_resolution_from_type(clockid_t c) {
  struct timespec t;
  if (branch_unexpected(clock_getres(c, &t))) {
    return -1;
  }
  return t.tv_sec * 1000 * 1000 * 1000 + t.tv_nsec;
}

int64_t gthread_clock_real() { return posix_time_from_type(CLOCK_REALTIME); }

int64_t gthread_clock_resolution_real() {
  return posix_time_resolution_from_type(CLOCK_REALTIME);
}

int64_t gthread_clock_monotonic() {
  return posix_time_from_type(CLOCK_MONOTONIC);
}

int64_t gthread_clock_resolution_monotonic() {
  return posix_time_resolution_from_type(CLOCK_MONOTONIC);
}

int64_t gthread_clock_process() {
  return posix_time_from_type(CLOCK_PROCESS_CPUTIME_ID);
}

int64_t gthread_clock_resolution_process() {
  return posix_time_resolution_from_type(CLOCK_PROCESS_CPUTIME_ID);
}

uint64_t gthread_nsleep(uint64_t ns) {
  struct timespec t = { ns / (1000*1000*1000), ns % (1000*1000*1000) };
  if (branch_unexpected(nanosleep(&t, &t))) {
    return ns - t.tv_sec * 1000 * 1000 * 1000 - t.tv_nsec;
  } else {
    return t.tv_sec * 1000 * 1000 * 1000 + t.tv_nsec;
  }
}
