#pragma once

#include <atomic>

#include "platform/alarm.h"
#include "platform/tls.h"
#include "util/compiler.h"

namespace gthread {
inline task* task::current() {
  // slow path on being the first call in this module
  if (branch_unexpected(!_is_root_task_init)) {
    init();
  }

  return (task*)gthread_tls_current_thread();
}

template <typename Duration>
void task::set_time_slice_trap(const time_slice_trap& trap, Duration interval) {
  _timer_enabled = interval.count() > 0;
  _time_slice_trap = trap;
  alarm::set_trap(&time_slice_trap_base);
  alarm::set_interval(interval);
}
}  // namespace gthread
