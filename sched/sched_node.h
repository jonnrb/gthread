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
      : _running(false),
        _interrupt_lock(UNLOCKED),
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

  /**
   * prevents preemption of the current task
   */
  void lock();

  /**
   * resumes preemption of the current task
   */
  void unlock();

  void schedule(task* t);

 private:
  std::atomic<bool> _running;

  enum interrupt_lock { UNLOCKED = 0, LOCKED_IN_SCHED, LOCKED_IN_TASK };
  std::atomic<interrupt_lock> _interrupt_lock;

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
