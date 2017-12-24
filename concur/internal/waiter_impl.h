#pragma once

#include "concur/internal/waiter.h"

#include "sched/preempt.h"
#include "sched/sched.h"

namespace gthread {
namespace internal {
template <typename CheckThunk>
bool waiter::park_if(CheckThunk&& check_thunk) {
  task* expected = nullptr;
  task* current = task::current();

  if (!_waiter.compare_exchange_strong(expected, current,
                                       std::memory_order_acquire)) {
    return false;
  }

  if (!check_thunk()) {
    _waiter.store(nullptr, std::memory_order_release);
    return false;
  }

  current->run_state = task::WAITING;
  sched::yield();

  return true;
}
}  // namespace internal
}  // namespace gthread
