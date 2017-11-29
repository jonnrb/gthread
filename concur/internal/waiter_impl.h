#pragma once

#include "concur/internal/waiter.h"

#include "sched/sched.h"

namespace gthread {
namespace internal {
template <typename CheckThunk>
bool waiter::park_if(CheckThunk&& check_thunk) {
  task* expected = nullptr;
  task* current = task::current();

  if (!_waiter.compare_exchange_strong(expected, current)) {
    return false;
  }

  if (!check_thunk()) {
    _waiter.store(nullptr);
    return false;
  }

  current->run_state = task::WAITING;
  auto& s = sched::get();
  s.yield();

  // `swap()` disables preemption
  s.unlock();

  return true;
}
}  // namespace internal
}  // namespace gthread
