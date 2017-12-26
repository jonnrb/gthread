#pragma once

#include <atomic>

#include "platform/clock.h"
#include "sched/internal/rq.h"
#include "sched/internal/task_freelist.h"
#include "sched/task.h"
#include "util/compiler.h"

namespace gthread {
class sched_node {
 public:
  sched_node(std::shared_ptr<internal::task_freelist> task_freelist)
      : _spin_lock(),
        _running(false),
        _deferred(nullptr),
        _rq(),
        _task_freelist(task_freelist),
        _host_task() {}

  /**
   * chooses a new task to run from the runqueue
   */
  void yield() mt_locks_excluded(_spin_lock);

  /**
   * chooses a new task to run from the runqueue and returns after at least
   * |duration| time has passed
   */
  void yield_for(internal::rq::sleepqueue_clock::duration duration)
      mt_locks_excluded(_spin_lock);

  /**
   * puts the current task on the runqueue and resumes control (now yield will
   * do stuff)
   */
  void start_async() mt_locks_excluded(_spin_lock);

  /**
   * does not put the current task on the runqueue (will not resume until
   * stopped)
   */
  void start() mt_locks_excluded(_spin_lock);

  // TODO
  // void stop();

  void schedule(task* t) mt_locks_excluded(_spin_lock);

  void switch_to(task* t) mt_locks_excluded(_spin_lock);

  /**
   * returns the sched_node hosting the current execution context (or nullptr)
   *
   * *should* be gotten from `preempt_mutex` instead to guarantee safety
   */
  static sched_node* current();

 private:
  /**
   * pushes |current| to the runqueue (if running) and pops the next eligible
   * task, possibly waiting until its sleep timer has expired
   */
  task* get_next_task(task* current) mt_locks_required(_spin_lock);

  // NOTE: no backoff and no fallback
  class mt_capability("mutex") spin_lock {
   public:
    spin_lock() : _flag(false) {}
    bool try_lock() mt_try_acquire(true);
    void lock() mt_acquire();
    void unlock() mt_release();

   private:
    std::atomic<bool> _flag;
  };
  using guard = std::lock_guard<spin_lock>;
  spin_lock _spin_lock;

  std::atomic<bool> _running;

  // task was detached and stopped, but we couldn't return it yet because we are
  // running on its stack (written by `get_next_task()` and read by `yield()`)
  task* _deferred mt_guarded_by(_spin_lock);

  internal::rq _rq mt_guarded_by(_spin_lock);

  using vruntime_clock = thread_clock;
  vruntime_clock::time_point _last_tick mt_guarded_by(_spin_lock);

  std::shared_ptr<internal::task_freelist> _task_freelist;

  task _host_task;
};
}  // namespace gthread
