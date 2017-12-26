#pragma once

#include <atomic>

#include "sched/preempt.h"
#include "util/log.h"

namespace gthread {
template <class Rep, class Period>
void sched::sleep_for(
    const std::chrono::duration<Rep, Period>& sleep_duration) {
  auto& pmu = preempt_mutex::get();
  std::lock_guard<preempt_mutex> l(pmu);
  pmu.node().yield_for(
      std::chrono::duration_cast<internal::rq::sleepqueue_clock::duration>(
          sleep_duration));
}
}  // namespace gthread
