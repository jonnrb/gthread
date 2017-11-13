#pragma once

#include "gthread.h"

#include "sched/sched.h"

namespace gthread {
template <typename Function, typename... Args>
void* g_polymorphic_start(void* arg) {
  auto* closure = static_cast<function_marshall<Function, Args...>*>(arg);
  (*closure)();
  return nullptr;
}

template <typename Function, typename... Args>
g::g(Function&& function, Args&&... args)
    : _closure(make_unique_function_marshall(std::forward<Function>(function),
                                             std::forward<Args>(args)...)) {
  sched& s = sched::get();
  _handle = s.spawn(k_light_attr, *g_polymorphic_start<Function, Args...>,
                    static_cast<void*>(_closure.get()));
}

g::~g() {}

void g::join() {
  auto& s = sched::get();
  s.join(&_handle, nullptr);
}

inline void self::yield() { sched::get().yield(); }

template <class Rep, class Period>
inline void self::sleep_for(
    const std::chrono::duration<Rep, Period>& sleep_duration) {
  sched::get().sleep_for(sleep_duration);
}
}  // namespace gthread
