#include "sched/internal/rq.h"

#include "util/log.h"

namespace gthread {
namespace internal {
template <typename Thunk>
task* rq::pop(const Thunk& idle_thunk) {
  // NOTE: it may be more cache efficient to bulk remove items from the
  // sleepqueue
  if (!_sleepqueue.empty()) {
    sleepqueue_clock::time_point next_runnable{};
    auto* sleeper = _sleepqueue.pop(&next_runnable);
    if (sleeper != nullptr) {
      _runqueue.emplace(sleeper);
    } else if (_runqueue.empty()) {
      // XXX: remove this if there other kinds of externally wakeable tasks
      idle_thunk(next_runnable);
      assert(sleepqueue_clock::now() >= next_runnable);
      _runqueue.emplace(sleeper);
    }
  } else if (_runqueue.empty()) {
    gthread_log_fatal("nothing to do. deadlock?");
  }

  auto begin = _runqueue.begin();
  task* next_task = *begin;
  _runqueue.erase(begin);

  // the task that was just popped from the `runqueue` a priori is the task
  // with the minimum vruntime. since new tasks must start with a reasonable
  // vruntime, update `min_vruntime`.
  if (_min_vruntime.load() < next_task->vruntime) {
    _min_vruntime.store(next_task->vruntime);
  }

  return next_task;
}
}  // namespace internal
}  // namespace gthread
