#include "sched/sched_node.h"

#include <mutex>
#include <thread>

#include "util/log.h"

namespace gthread {
namespace {
thread_local sched_node* g_current_sched_node = nullptr;
}

using guard = std::lock_guard<sched_node>;

sched_node* sched_node::current() { return g_current_sched_node; }

void sched_node::start_async() {
  g_current_sched_node = this;

  _last_tick = vruntime_clock::now();

  bool expected = false;
  if (!_running.compare_exchange_strong(expected, true)) {
    gthread_log_fatal("sched_node was already running");
  }
}

void sched_node::start() {
  bool expected = false;
  if (!_running.compare_exchange_strong(expected, true)) {
    gthread_log_fatal("sched_node was already running");
  }

  _last_tick = vruntime_clock::now();

  yield();  // will not resume until stopped

  if (_running.load()) {
    gthread_log_fatal(
        "control resumed in host task before sched_node stopped and sched_node "
        "was started synchronously. there is either a bug in the scheduler or "
        "there was nothing initially scheduled when start() was called (user "
        "error)");
  }
}

void sched_node::yield() {
  if (!_running.load()) return;

  auto* cur = task::current();

  auto now = vruntime_clock::now();
  cur->vruntime +=
      std::chrono::duration_cast<decltype(cur->vruntime)>(now - _last_tick);
  _last_tick = now;

  // if for some reason, a task that was about to switch gets interrupted,
  // don't switch tasks
  auto expected = UNLOCKED;
  if (branch_unexpected(!_interrupt_lock.compare_exchange_strong(
          expected, LOCKED_IN_SCHED, std::memory_order_acquire))) {
    if (expected != LOCKED_IN_TASK) {
      gthread_log_fatal(
          "scheduler was interrupted by itself! this should be impossible. "
          "bug???");
    }
    return;
  }

  // if the task was in a runnable state when the scheduler was invoked, push it
  // to the runqueue
  if (cur->run_state == task::RUNNING) {
    _rq.push(cur);
  } else if (cur->run_state == task::STOPPED && cur->detached) {
    _task_freelist->return_task_from_scheduler(cur);
  }

  static const std::function<void(internal::rq::sleepqueue_clock::time_point)>
      idle_thunk = [](internal::rq::sleepqueue_clock::time_point wake_time) {
        std::this_thread::sleep_until(wake_time);
      };
  task* next_task = _rq.pop(idle_thunk);

  _interrupt_lock.store(UNLOCKED, std::memory_order_release);

  if (next_task != cur) {
    next_task->switch_to([this]() { g_current_sched_node = this; });
  }
}

void sched_node::yield_for(internal::rq::sleepqueue_clock::duration duration) {
  auto* current = task::current();

  {
    std::lock_guard<sched_node> l(*this);
    current->run_state = task::WAITING;
    _rq.sleep_push(current, duration);
  }

  yield();
}

void sched_node::lock() {
  auto expected = UNLOCKED;
  if (!branch_unexpected(_interrupt_lock.compare_exchange_strong(
          expected, LOCKED_IN_TASK, std::memory_order_acquire))) {
    gthread_log_fatal("contention on scheduler from uninterruptable code!");
  }
}

void sched_node::unlock() {
  _interrupt_lock.store(UNLOCKED, std::memory_order_release);
}

void sched_node::schedule(task* t) {
  // initialize the vruntime if |t| is a new task
  if (t->vruntime == std::chrono::microseconds{0}) {
    t->vruntime = _rq.min_vruntime();
  }

  _rq.push(t);
}
}  // namespace gthread
