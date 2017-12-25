#pragma once

#include <cstdint>
#include <cstdlib>

#include "sched/sched.h"
#include "sched/task_attr.h"
#include "util/function_marshall.h"

namespace gthread {
class g {
 public:
  /**
   * constructs an empty object not corresponding to any execution context
   */
  g() : _handle() {}

  /**
   * if this represents an execution context and is destructed, it will be
   * detached
   */
  ~g() { detach(); }

  /**
   * creates a new execution context running |function| which will be passed
   * all of the args in |args...|
   *
   * the created object will represent that execution context
   */
  template <typename Function, typename... Args>
  explicit g(Function&& function, Args&&... args);

  /**
   * blocks until the underlying execution context has finished running
   *
   * this must have an associated execution context
   */
  inline void join();

  /**
   * lets the execution context know that nothing will `join()` it
   *
   * this exists for compatability with `std::thread`. this can just be left
   * to destruct and that will detach.
   */
  inline void detach();

  /**
   * returns if this represents an execution context
   */
  operator bool() { return _handle; }

 private:
  sched::handle _handle;
};

class self {
 public:
  /**
   * suspends execution to possibly schedule another execution context
   */
  inline static void yield();

  /**
   * suspends execution for a fixed amount of time
   */
  template <class Rep, class Period>
  inline static void sleep_for(
      const std::chrono::duration<Rep, Period>& sleep_duration);
};
}  // namespace gthread

#include "gthread_impl.h"
