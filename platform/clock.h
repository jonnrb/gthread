#ifndef PLATFORM_CLOCK_H_
#define PLATFORM_CLOCK_H_

#include <stdint.h>

/**
 * "real" time (i.e. time on the wall time)
 */
int64_t gthread_clock_real();

int64_t gthread_clock_resolution_real();

/**
 * time value that is monotonically increasing in the system
 */
int64_t gthread_clock_monotonic();

int64_t gthread_clock_resolution_monotonic();

/**
 * time that increases while the process is running
 */
int64_t gthread_clock_process();

int64_t gthread_clock_resolution_process();

/**
 * sleep the kernel process for |ns| nanoseconds
 */
uint64_t gthread_nsleep(uint64_t ns);

#endif  // PLATFORM_CLOCK_H_
