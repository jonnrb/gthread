/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//platform/timer.h
 * info: platform timer utilities
 */

#ifndef SCHED_TIME_SLICE_H_
#define SCHED_TIME_SLICE_H_

#include <stdint.h>
#include <time.h>

// an interval of 0 µs disables the timer.
int gthread_timer_set_interval(uint64_t usec);

// resets the interval and returns the elapsed time.
uint64_t gthread_timer_reset();

int gthread_timer_alarm_now();

typedef void gthread_timer_trap_t(uint64_t elapsed);

void gthread_timer_set_trap(gthread_timer_trap_t* trap);

#endif  // SCHED_TIME_SLICE_H_
