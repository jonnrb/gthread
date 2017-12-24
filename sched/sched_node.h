#pragma once

#include <atomic>

#include "platform/clock.h"
#include "sched/internal/rq.h"
#include "sched/internal/task_freelist.h"
#include "sched/task.h"

namespace gthread {
class sched_node {
 public:
  sched_node(std::shared_ptr<internal::task_freelist> task_freelist)
      : interrupt_lock(),
        _running(false),
        _rq(),
        _task_freelist(task_freelist),
        _host_task(task::create_wrapped()) {}

  /**
   * returns the sched_node hosting the current execution context (or nullptr)
   */
  static sched_node* current();

  /**
   * chooses a new task to run from the runqueue
   */
  void yield();

  /**
   * chooses a new task to run from the runqueue and returns after at least
   * |duration| time has passed
   */
  void yield_for(internal::rq::sleepqueue_clock::duration duration);

  /**
   * puts the current task on the runqueue and resumes control (now yield will
   * do stuff)
   */
  void start_async();

  /**
   * does not put the current task on the runqueue (will not resume until
   * stopped)
   */
  void start();

  // TODO
  // void stop();

  void schedule(task* t);

 private:
  class spin_lock {
   public:
    spin_lock() : _flag(false) {}
    bool try_lock();
    void lock();
    void unlock();

   private:
    std::atomic_flag _flag;
  };

 public:
  spin_lock interrupt_lock;

 private:
  std::atomic<bool> _running;

  /**
   * runqueue
   */
  internal::rq _rq;

  using vruntime_clock = thread_clock;
  vruntime_clock::time_point _last_tick;

  /**
   * place to return finished tasks to (ideally this shouldn't be done here
   * in the first place...)
   */
  std::shared_ptr<internal::task_freelist> _task_freelist;

  task _host_task;
};
}  // namespace gthread
