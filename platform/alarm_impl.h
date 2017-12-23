#pragma once

#include "platform/alarm.h"

#include "util/compiler.h"

namespace gthread {
template <typename Duration>
void alarm::set_interval(Duration interval) {
  set_interval_impl(
      std::chrono::duration_cast<std::chrono::microseconds>(interval));
}

template <typename Function>
void alarm::set_trap(Function&& f) {
  bool set_trap_now = !_trap;
  _trap = std::forward<Function>(f);
  if (branch_unexpected(set_trap_now)) {
    set_signal();
  }
}
}  // namespace gthread
