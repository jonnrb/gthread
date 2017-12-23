#pragma once

#include <atomic>
#include <list>
#include <map>
#include <set>

#include "sched/internal/task_freelist.h"
#include "sched/task.h"
#include "sched/task_attr.h"

namespace gthread {
namespace sched {
/**
 * a handle to a currently running task
 *
 * example: ```
 *   gthread::sched::handle task =
 *       gthread::sched::spawn(nullptr, my_function, arg);
 *   gthread::sched::join(&task, NULL);
 * ```
 */
class handle {
 public:
  constexpr handle() : t(nullptr) {}
  constexpr operator bool() const { return t != nullptr; }

 private:
  task* t;

  friend handle spawn(const attr& attr, task::entry_t* entry, void* arg);
  friend void join(handle* handle, void** return_value);
  friend void detach(handle* handle);
  friend void exit(void* return_value);
};

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
handle spawn(const attr& a, task::entry_t entry, void* arg);

/**
 * detaches from |task|, making it unjoinable. |task| will return its
 * resources upon completion.
 */
void detach(handle* task);

/**
 * joins |task| when it finishes running (the caller will be suspended)
 *
 * |return_value| can be NULL. when |return_value| is not NULL,
 * `*return_value` will contain the return value of |task|, either from the
 * returning entry point, or passed in via `gthread_sched_exit()`.
 */
void join(handle* task, void** return_value);

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
inline void sleep_for(const std::chrono::duration<Rep, Period>& sleep_duration);
}  // namespace sched
}  // namespace gthread

#include "sched/sched_impl.h"
