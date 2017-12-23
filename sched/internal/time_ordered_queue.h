#pragma once

#include <map>
#include <shared_mutex>

namespace gthread {
namespace internal {
template <typename T, typename Clock>
class time_ordered_queue {
 public:
  time_ordered_queue() : _q() {}

  template <typename U>
  void push(typename Clock::time_point ripe, U&& u);

  /**
   * if there is something on the queue that is ripe, return it. if |ripe| is
   * specified, the first thing on the queue's ripe time is written there.
   */
  T pop(typename Clock::time_point* ripe);

  constexpr bool empty() const { return _q.empty(); }

 private:
  std::multimap<typename Clock::time_point, T> _q;
};
}  // namespace internal
}  // namespace gthread

#include "sched/internal/time_ordered_queue_impl.h"
