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

namespace gthread {
inline void sched::uninterruptable_lock() {
  task* current = task::current();

  task* expected = nullptr;
  if (!branch_unexpected(
          interrupt_lock.compare_exchange_strong(expected, current))) {
    gthread_log_fatal("contention on scheduler from uninterruptable code!");
  }
}

inline void sched::uninterruptable_unlock() { interrupt_lock = nullptr; }

inline void sched::runqueue_push(task* t) { runqueue.emplace(t); }
}  // namespace gthread

#endif  // SCHED_SCHED_INLINE_H_
