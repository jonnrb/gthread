/**
 * author: JonNRb <jonbetti@gmail.com>, Matthew Handzy <matthewhandzy@gmail.com>
 * license: MIT
 * file: @gthread//sched/sched_line.h
 * info: scheduler for uniprocessors (inlined code)
 */

#ifndef SCHED_SCHED_INLINE_H_
#define SCHED_SCHED_INLINE_H_

#include <atomic>

#include "util/log.h"

static inline void gthread_sched_uninterruptable_lock() {
  extern std::atomic<gthread_task_t*> g_interrupt_lock;
  gthread_task_t* current = gthread_task_current();

  gthread_task_t* expected = nullptr;
  if (!branch_unexpected(
          g_interrupt_lock.compare_exchange_strong(expected, current))) {
    gthread_log_fatal("contention on rb tree from uninterruptable code!");
  }
}

static inline void gthread_sched_uninterruptable_unlock() {
  extern std::atomic<gthread_task_t*> g_interrupt_lock;
  g_interrupt_lock = nullptr;
}

#endif  // SCHED_SCHED_INLINE_H_
