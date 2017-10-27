#pragma once

#include <type_traits>

#include "gthread.h"
#include "platform/clock.h"
#include "platform/timer.h"
#include "sched/sched.h"

namespace gthread {
namespace internal {
/**
 * when enabled, allows collection of stats for the scheduler. when disabled,
 * actions should dissolve.
 */
template <bool enabled = false>
class sched_stats {
 public:
  sched_stats(thread_clock::duration interval);

  struct dummy_timer {
    dummy_timer(timer<thread_clock>::end_function) {}
  };
  using stats_timer = typename std::conditional<enabled, timer<thread_clock>,
                                                dummy_timer>::type;

  stats_timer get_timer() {
    return stats_timer(
        [this](thread_clock::duration duration) { _used_time += duration; });
  }

  int init();

 private:
  void print();

  static void* thread(void* arg) {
    sched_stats<enabled>* self = (sched_stats<enabled>*)arg;
    self->print();
    return nullptr;
  }

  thread_clock::time_point _start_time;
  thread_clock::duration _used_time;
  thread_clock::duration _interval;
  sched_handle _self_handle;
};

#ifdef GTHREAD_SCHED_COLLECT_STATS

static constexpr bool k_collect_stats = true;
#ifndef GTHREAD_SCHED_STATS_INTERVAL_MILLISECONDS
static constexpr std::chrono::seconds k_stats_interval{5};
#else
static constexpr std::chrono::milliseconds k_stats_interval{
    GTHREAD_SCHED_STATS_INTERVAL_MILLISECONDS};
#endif  // GTHREAD_SCHED_STATS_INTERVAL_MILLISECONDS

#else

static constexpr bool k_collect_stats = false;
static constexpr std::chrono::seconds k_stats_interval{0};

#endif  // GTHREAD_SCHED_COLLECT_STATS

}  // namespace internal
}  // namespace gthread

#include "sched/stats_impl.h"
