/**
 * author: JonNRb <jonbetti@gmail.com>, Matthew Handzy <matthewhandzy@gmail.com>
 * license: MIT
 * file: @gthread//sched/sched_line.h
 * info: scheduler for uniprocessors (inlined code)
 */

#ifndef SCHED_SCHED_INLINE_H_
#define SCHED_SCHED_INLINE_H_

#include <stdio.h>

#include "arch/atomic.h"

static inline void gthread_sched_uninterruptable_lock() {
  extern uint64_t g_interrupt_lock;
  gthread_task_t* current = gthread_task_current();

  if (branch_unexpected(
          !gthread_cas(&g_interrupt_lock, 0, (uint64_t)current))) {
    printf("contention on rb tree from uninterruptable code!\n");
    abort();
  }
}

static inline void gthread_sched_uninterruptable_unlock() {
  extern uint64_t g_interrupt_lock;
  g_interrupt_lock = 0;
}

#endif  // SCHED_SCHED_INLINE_H_
