#pragma once

#include <chrono>
#include <set>

#include "sched/internal/time_ordered_queue.h"
#include "sched/task.h"

namespace gthread {
namespace internal {
class rq {
 public:
  void push(task* t);

  using sleepqueue_clock = std::chrono::system_clock;

  void sleep_push(task* t, sleepqueue_clock::duration sleep_duration);

  template <typename Thunk>
  task* pop(const Thunk& idle_thunk);

  constexpr std::chrono::microseconds min_vruntime() const {
    return _min_vruntime.load();
  }

 private:
  /**
   * tasks that can be switched to with the expectation that they will make
   * progress
   *
   * `std::multiset` is used because it usually is built on a pretty robust
   * red-black tree, which has good performance for a runqueue
   */
  struct time_ordered_compare {
    constexpr bool operator()(const task* a, const task* b) const {
      return a->vruntime != b->vruntime ? a->vruntime < b->vruntime : a < b;
    }
  };
  std::multiset<task*, time_ordered_compare> _runqueue;

  time_ordered_queue<task*, sleepqueue_clock> _sleepqueue;

  std::atomic<std::chrono::microseconds> _min_vruntime;
};
};  // namespace internal
};  // namespace gthread

#include "sched/internal/rq_impl.h"
