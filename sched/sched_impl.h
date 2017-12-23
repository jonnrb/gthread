#pragma once

#include <atomic>

#include "sched/sched_node.h"
#include "util/log.h"

namespace gthread {
template <class Rep, class Period>
void sched::sleep_for(
    const std::chrono::duration<Rep, Period>& sleep_duration) {
  auto* node = sched_node::current();
  node->yield_for(
      std::chrono::duration_cast<internal::rq::sleepqueue_clock::duration>(
          sleep_duration));
}
}  // namespace gthread
