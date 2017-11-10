#pragma once

#include <atomic>

#include "util/log.h"

namespace gthread {
inline void sched::lock() {
  task* current = task::current();

  task* expected = nullptr;
  if (!branch_unexpected(
          _interrupt_lock.compare_exchange_strong(expected, current))) {
    gthread_log_fatal("contention on scheduler from uninterruptable code!");
  }
}

inline void sched::unlock() { _interrupt_lock = nullptr; }

inline void sched::runqueue_push(task* t) { _runqueue.emplace(t); }

template <class Rep, class Period>
void sched::sleep_for(
    const std::chrono::duration<Rep, Period>& sleep_duration) {
  sleep_for_impl(
      std::chrono::duration_cast<sleepqueue_clock::duration>(sleep_duration));
}
}  // namespace gthread
