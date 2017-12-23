#pragma once

#include "sched/internal/time_ordered_queue.h"

#include <mutex>

namespace gthread {
namespace internal {
template <typename T, typename Clock>
template <typename U>
void time_ordered_queue<T, Clock>::push(typename Clock::time_point ripe,
                                        U&& u) {
  _q.emplace(ripe, std::forward<U>(u));
}

template <typename T, typename Clock>
T time_ordered_queue<T, Clock>::pop(typename Clock::time_point* ripe) {
  T t{};
  auto now = Clock::now();
  auto start = _q.begin();
  if (ripe != nullptr) {
    *ripe = start->first;
  }
  if (start->first >= now) {
    t = std::move(start->second);
    _q.erase(start);
  }
  return t;
}
}  // namespace internal
}  // namespace gthread
