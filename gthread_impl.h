#pragma once

#include "gthread.h"

#include <stdexcept>

#include "sched/sched.h"
#include "util/compiler.h"

namespace gthread {
template <typename Function, typename... Args>
void* g_polymorphic_start(void* arg) {
  using fm = function_marshall<Function, Args...>;
  auto closure = std::unique_ptr<fm>(static_cast<fm*>(arg));
  (*closure)();
  return nullptr;
}

template <typename Function, typename... Args>
g::g(Function&& function, Args&&... args) {
  sched& s = sched::get();
  auto closure = make_unique_function_marshall(std::forward<Function>(function),
                                               std::forward<Args>(args)...);
  // XXX: potential leak at `closure.release()`
  _handle = s.spawn(k_light_attr, *g_polymorphic_start<Function, Args...>,
                    static_cast<void*>(closure.release()));
}

void g::join() {
  if (branch_unexpected(!_handle)) {
    throw std::logic_error("no execution context to join");
  }
  sched::get().join(&_handle, nullptr);
}

void g::detach() {
  if (!_handle) return;
  sched::get().detach(&_handle);
}

void self::yield() { sched::get().yield(); }

template <class Rep, class Period>
void self::sleep_for(const std::chrono::duration<Rep, Period>& sleep_duration) {
  sched::get().sleep_for(sleep_duration);
}
}  // namespace gthread
