#pragma once

#include <atomic>

#include "util/log.h"

namespace gthread {
inline void sched::uninterruptable_lock() {
  task* current = task::current();

  task* expected = nullptr;
  if (!branch_unexpected(
          _interrupt_lock.compare_exchange_strong(expected, current))) {
    gthread_log_fatal("contention on scheduler from uninterruptable code!");
  }
}

inline void sched::uninterruptable_unlock() { _interrupt_lock = nullptr; }

inline void sched::runqueue_push(task* t) { _runqueue.emplace(t); }
}  // namespace gthread
