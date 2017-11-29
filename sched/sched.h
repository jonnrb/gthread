#pragma once

#include <atomic>
#include <list>
#include <map>
#include <set>

#include "sched/task.h"
#include "sched/task_attr.h"
#include "sched/task_freelist.h"

namespace gthread {
/**
 * a handle to a currently running task
 *
 * example: ```
 *   gthread::sched_handle task =
 *       gthread::sched::spawn(nullptr, my_function, arg);
 *   gthread::sched::join(&task, NULL);
 * ```
 */
class sched_handle {
 public:
  constexpr sched_handle() : t(nullptr) {}
  constexpr operator bool() const { return t != nullptr; }

 private:
  task* t;

  friend class sched;
};

class sched {
 public:
  /**
   * returns the current task's scheduler
   */
  static sched& get();

  /**
   * invokes the current task's scheduler which picks a new task to run
   *
   * the scheduler will pick the task that has had the minimum amount of
   * proportional processor time and run that task. when the current task meets
   * that condition, it will be resume on the line following this call.
   */
  void yield();

  /**
   * spawns a gthread, storing a handle in |handle|, where |entry| will be
   * invoked with argument |arg|. `gthread::sched::join()` must be called
   * eventually to clean up the called thread when it finishes
   */
  sched_handle spawn(const attr& a, task::entry_t entry, void* arg);

  /**
   * detaches from |task|, making it unjoinable. |task| will return its
   * resources upon completion.
   */
  void detach(sched_handle* task);

  /**
   * joins |task| when it finishes running (the caller will be suspended)
   *
   * |return_value| can be NULL. when |return_value| is not NULL,
   * `*return_value` will contain the return value of |task|, either from the
   * returning entry point, or passed in via `gthread_sched_exit()`.
   */
  void join(sched_handle* task, void** return_value);

  /**
   * ends the current task with return value |return_value|
   *
   * the current task normally ends by returning a value from its entry point.
   * however, if it must end abruptly, this function must be used.
   */
  void exit(void* return_value);

  /**
   * sleeps the current task until |sleep_duration| has passed
   */
  template <class Rep, class Period>
  inline void sleep_for(
      const std::chrono::duration<Rep, Period>& sleep_duration);

  /**
   * disables preemption by the scheduler
   *
   * helper function for things that implement cooperative functionality
   * to make themselves uninterruptable for the *only* purpose of marking the
   * current task as waiting and putting it on a waitqueue of some kind
   *
   * this *disables* the scheduler in the sense that when the scheduler is
   * invoked, the locking task will be resumed
   */
  inline void lock();

  /**
   * resumes preemption by the scheduler if it has been disabled
   *
   * if `sched::get().lock()` is called, this must be called to allow preemption
   * to happen again
   */
  inline void unlock();

  void enable_timer_preemption();

  void disable_timer_preemption();

  inline void runqueue_push(task* t);

 private:
  sched();

  sched(sched&) = delete;

  using sleepqueue_clock = std::chrono::system_clock;
  void sleep_for_impl(sleepqueue_clock::duration sleep_duration);

  task* next(task* last_running_task);

  enum interrupt_lock { UNLOCKED = 0, LOCKED_IN_SCHED, LOCKED_IN_TASK };
  std::atomic<interrupt_lock> _interrupt_lock;

  /**
   * tasks that can be switched to with the expectation that they will make
   * progress
   *
   * implementation details:
   *
   * `std::set` is used because it usually is built on a pretty robust
   * red-black tree, which has good performance for a runqueue
   */
  struct time_ordered_compare {
    constexpr bool operator()(const task* a, const task* b) const {
      return a->vruntime != b->vruntime ? a->vruntime < b->vruntime : a < b;
    }
  };
  std::set<task*, time_ordered_compare> _runqueue;

  // tasks that will be woken up after a point in time
  std::multimap<sleepqueue_clock::time_point, task*> _sleepqueue;

  // default age for new tasks
  std::chrono::microseconds _min_vruntime;

  task_freelist _freelist;
};
}  // namespace gthread

#include "sched/sched_impl.h"
