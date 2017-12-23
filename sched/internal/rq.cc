#include "sched/internal/rq.h"

#include <cassert>

#include "util/log.h"

namespace gthread {
namespace internal {
void rq::push(task* t) { _runqueue.emplace(t); }

void rq::sleep_push(task* t, sleepqueue_clock::duration sleep_duration) {
  auto earliest_wake_time = sleepqueue_clock::now() + sleep_duration;
  auto* cur = task::current();

  cur->run_state = task::WAITING;
  _sleepqueue.push(earliest_wake_time, cur);
}
}  // namespace internal
}  // namespace gthread
