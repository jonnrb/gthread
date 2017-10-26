/**
 * author: JonNRb <jonbetti@gmail.com>, Matthew Handzy <matthewhandzy@gmail.com>
 * license: MIT
 * file: @gthread//sched/sched.h
 * info: scheduler for uniprocessors
 */

#ifndef SCHED_SCHED_H_
#define SCHED_SCHED_H_

#include <atomic>
#include <set>

#include "gthread.h"
#include "sched/task.h"

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
  sched() = delete;

  /**
   * stops the current task and invokes the scheduler
   *
   * the scheduler will pick the task that has had the minimum amount of
   * proportional processor time and run that task. when the current task meets
   * that condition, it will be resume on the line following this call.
   */
  static int yield();

  /**
   * spawns a gthread, storing a handle in |handle|, where |entry| will be
   * invoked with argument |arg|. `gthread::sched::join()` must be called
   * eventually to clean up the called thread when it finishes
   *
   * TODO: sched::detach()
   */
  static sched_handle spawn(const attr& a, task::entry_t entry, void* arg);

  /**
   * joins |task| when it finishes running (the caller will be suspended)
   *
   * |return_value| can be NULL. when |return_value| is not NULL,
   * `*return_value` will contain the return value of |task|, either from the
   * returning entry point, or passed in via `gthread_sched_exit()`.
   */
  static int join(sched_handle* task, void** return_value);

  /**
   * ends the current task with return value |return_value|
   *
   * the current task normally ends by returning a value from its entry point.
   * however, if it must end abruptly, this function must be used.
   */
  static void exit(void* return_value);

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
  static inline void uninterruptable_lock();

  /**
   * resumes preemption by the scheduler if it has been disabled
   *
   * if `gthread_sched_uninterruptable_lock()` is called, this must be called to
   * allow preemption to happen again
   */
  static inline void uninterruptable_unlock();

  static inline void runqueue_push(task* t);

 private:
  static task* make_task(const attr& a);

  static void return_task(task* task);

  static int init();

  static task* next(task* last_running_task);

  static std::atomic<bool> is_init;

  static task* const k_pointer_lock;
  static std::atomic<task*> interrupt_lock;

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
  static std::set<task*, time_ordered_compare> runqueue;

  static uint64_t min_vruntime;

  // ring buffer for free tasks
  static constexpr uint64_t k_freelist_size = 64;
  static uint64_t freelist_r;  // reader is `make_task()`
  static uint64_t freelist_w;  // writer is `return_task()`
  static task* freelist[k_freelist_size];
};
}  // namespace gthread

#include "sched/sched_inline.h"

#endif  // SCHED_SCHED_H_
