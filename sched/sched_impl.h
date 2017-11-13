#pragma once

#include <atomic>

#include "util/log.h"

namespace gthread {
inline void sched::lock() {
  interrupt_lock expected = UNLOCKED;
  if (!branch_unexpected(_interrupt_lock.compare_exchange_strong(
          expected, LOCKED_IN_TASK, std::memory_order_acquire))) {
    gthread_log_fatal("contention on scheduler from uninterruptable code!");
  }
}

inline void sched::unlock() {
  _interrupt_lock.store(UNLOCKED, std::memory_order_release);
}

inline void sched::runqueue_push(task* t) { _runqueue.emplace(t); }

template <class Rep, class Period>
void sched::sleep_for(
    const std::chrono::duration<Rep, Period>& sleep_duration) {
  sleep_for_impl(
      std::chrono::duration_cast<sleepqueue_clock::duration>(sleep_duration));
}
}  // namespace gthread
