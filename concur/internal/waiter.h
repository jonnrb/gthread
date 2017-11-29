#pragma once

#include <atomic>

#include "sched/task.h"

namespace gthread {
namespace internal {
/**
 * lets you park a thread outside of the runqueue
 */
class waiter {
 public:
  waiter() : _waiter(nullptr) {}

  /**
   * returns true if there is a parked exeuction context
   */
  operator bool() { return _waiter.load() != nullptr; }

  /**
   * suspends current execution context. returns true if parking was successful.
   */
  bool park();

  /**
   * suspends current execution context if |check_thunk| returns true
   *
   * |check_thunk| is only called if no other context is already parked or in
   * the process of parking on this waiter. if |check_thunk| returns false or
   * the current context cannot be parked, this will return false.
   */
  template<typename CheckThunk>
  bool park_if(CheckThunk&& check_thunk);

  /**
   * if there was an execution context waiting, it will be unparked to the
   * runqueue.
   */
  bool unpark();

  /**
   * if there was an execution context waiting, it will be resumed and this
   * execution context will be suspended
   */
  bool swap();

 private:
  std::atomic<task*> _waiter;
};
}  // namespace internal
}  // namespace gthread

#include "concur/internal/waiter_impl.h"
