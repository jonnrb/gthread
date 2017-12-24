#include "sched/sched_node.h"

#include <mutex>
#include <thread>

#include "util/log.h"

namespace gthread {
namespace {
thread_local sched_node* g_current_sched_node = nullptr;
}  // namespace

sched_node* sched_node::current() { return g_current_sched_node; }

void sched_node::start_async() mt_no_analysis {
  g_current_sched_node = this;

  _last_tick = vruntime_clock::now();
  _host_task.wrap_current();

  bool expected = false;
  if (!_running.compare_exchange_strong(expected, true)) {
    gthread_log_fatal("sched_node was already running");
  }
}

void sched_node::start() mt_no_analysis {
  _last_tick = vruntime_clock::now();
  _host_task.wrap_current();

  bool expected = false;
  if (!_running.compare_exchange_strong(expected, true)) {
    gthread_log_fatal("sched_node was already running");
  }

  yield();  // will not resume until stopped

  if (_running.load()) {
    gthread_log_fatal(
        "control resumed in host task before sched_node stopped and sched_node "
        "was started synchronously. there is either a bug in the scheduler or "
        "there was nothing initially scheduled when start() was called (user "
        "error)");
  }
}

class mt_scoped_capability try_lock_guard {
 public:
  try_lock_guard(sched_node::spin_lock& t) mt_acquire(t) : _t(t), _succ(false) {
    _succ = _t.try_lock();
  }

  ~try_lock_guard() mt_release() {
    if (_succ) _t.unlock();
  }

  void lock() mt_acquire(_t) { _t.lock(); }

  void unlock() mt_release(_t) { _t.unlock(); }

  operator bool() const { return _succ; }

 private:
  sched_node::spin_lock& _t;
  bool _succ;
};

void sched_node::yield() {
  if (!_running.load()) return;

  task* next_task;
  task* cur;
  {
    try_lock_guard l(_spin_lock);

    // BUG: this *could* be bad if a task uses `yield()` to deschedule itself,
    // but another execution context running concurrently locks `_spin_lock`...
    if (!l) return;

    if (current() != this) return;  // well, that was awkward

    cur = task::current();

    // update virtual runtime of currently running task
    auto now = vruntime_clock::now();
    cur->vruntime +=
        std::chrono::duration_cast<decltype(cur->vruntime)>(now - _last_tick);
    _last_tick = now;

    // if the task was in a runnable state when the scheduler was invoked, push
    // it to the runqueue
    if (cur->run_state == task::RUNNING) {
      _rq.push(cur);
    } else if (cur->run_state == task::STOPPED && cur->detached) {
      _task_freelist->return_task_from_scheduler(cur);
    }

    next_task =
        _rq.pop([&l](internal::rq::sleepqueue_clock::time_point wake_time)
                    mt_no_analysis {
                      l.unlock();
                      std::this_thread::sleep_until(wake_time);
                      l.lock();
                    });
  }

  if (next_task != cur) {
    next_task->switch_to([this]() { g_current_sched_node = this; });
  }
}

void sched_node::yield_for(internal::rq::sleepqueue_clock::duration duration) {
  auto* current = task::current();

  {
    std::lock_guard<spin_lock> l(_spin_lock);
    current->run_state = task::WAITING;
    _rq.sleep_push(current, duration);
  }

  yield();
}

bool sched_node::spin_lock::try_lock() {
  bool expected = false;
  return _flag.compare_exchange_strong(expected, true,
                                       std::memory_order_acquire);
}

void sched_node::spin_lock::lock() {
  // low level spin utilizing amd64 pause instruction between tries
  bool expected = false;
  while (
      !_flag.compare_exchange_weak(expected, true, std::memory_order_acquire)) {
    asm("pause");
    expected = false;
  }
}

void sched_node::spin_lock::unlock() {
  _flag.store(false, std::memory_order_release);
}

void sched_node::schedule(task* t) {
  std::lock_guard<spin_lock> l(_spin_lock);

  // initialize the vruntime if |t| is a new task
  if (t->vruntime == std::chrono::microseconds{0}) {
    t->vruntime = _rq.min_vruntime();
  }

  _rq.push(t);
}
}  // namespace gthread
