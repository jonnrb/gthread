#ifndef PLATFORM_CLOCK_H_
#define PLATFORM_CLOCK_H_

#include <stdint.h>

int64_t gthread_clock_real();

int64_t gthread_clock_resolution_real();

int64_t gthread_clock_monotonic();

int64_t gthread_clock_resolution_monotonic();

int64_t gthread_clock_process();

int64_t gthread_clock_resolution_process();

uint64_t gthread_nsleep(uint64_t ns);

#endif  // PLATFORM_CLOCK_H_
