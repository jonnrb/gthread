#pragma once

#include <cstdint>
#include <cstdlib>

#include "sched/sched.h"
#include "sched/task_attr.h"
#include "util/function_marshall.h"

namespace gthread {
class g {
 public:
  g();
  ~g();

  template <typename Function, typename... Args>
  explicit g(Function&& function, Args&&... args);

  void join();

  // void detach();

 private:
  std::unique_ptr<closure> _closure;
  sched_handle _handle;
};

class self {
 public:
  inline void yield();

  template <class Rep, class Period>
  inline void sleep_for(
      const std::chrono::duration<Rep, Period>& sleep_duration);
};
};  // namespace gthread

#include "gthread_impl.h"
