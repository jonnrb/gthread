#include "sched/sched.h"

#include <atomic>
#include <iostream>
#include <set>
#include <thread>

#include <errno.h>
#include <inttypes.h>

#include "platform/clock.h"
#include "platform/memory.h"
#include "util/compiler.h"
#include "util/log.h"

namespace gthread {
namespace {
void task_end_handler(task* task) { sched::get().exit(task->return_value); }
}  // namespace

task* const sched::k_pointer_lock = (task*)-1;

sched::sched() : _interrupt_lock(nullptr), _min_vruntime(0), _freelist(64) {
  task::set_time_slice_trap([this](task* current) { return next(current); },
                            std::chrono::milliseconds{50});
  task::set_end_handler(task_end_handler);
}

sched& sched::get() {
  static sched s;
  return s;
}

/**
 * pushes the |last_running_task| to the `runqueue` if it is in a runnable
 * state and pops the task with the least virtual runtime from `runqueue` to
 * return
 */
task* sched::next(task* last_running_task) {
  // if for some reason, a task that was about to switch gets interrupted,
  // switch to that task
  task* waiter = nullptr;
  if (branch_unexpected(
          !_interrupt_lock.compare_exchange_strong(waiter, k_pointer_lock))) {
    if (waiter == k_pointer_lock) {
      gthread_log_fatal(
          "scheduler was interrupted by itself! this should be impossible. "
          "bug???");
    }
    return waiter;
  }

  // if the task was in a runnable state when the scheduler was invoked, push it
  // to the runqueue
  if (last_running_task->run_state == task::RUNNING) {
    runqueue_push(last_running_task);
  }

  // NOTE: it may be more cache efficient to bulk remove items from the
  // sleepqueue
  if (!_sleepqueue.empty()) {
    auto sleeper = _sleepqueue.begin();
    if (sleeper->first <= sleepqueue_clock::now()) {
      _runqueue.emplace(sleeper->second);
      _sleepqueue.erase(sleeper);
    } else if (_runqueue.empty()) {
      // XXX: remove this if there other kinds of externally wakeable tasks
      _sleepqueue.erase(sleeper);
      std::this_thread::sleep_until(sleeper->first);
      _runqueue.emplace(sleeper->second);
    }
  } else if (_runqueue.empty()) {
    gthread_log_fatal("nothing to do. deadlock?");
  }

  auto begin = _runqueue.begin();
  task* next_task = *begin;
  _runqueue.erase(begin);

  _interrupt_lock = nullptr;

  // the task that was just popped from the `runqueue` a priori is the task
  // with the minimum vruntime. since new tasks must start with a reasonable
  // vruntime, update `min_vruntime`.
  if (_min_vruntime < next_task->vruntime) {
    _min_vruntime = next_task->vruntime;
  }

  return next_task;
}

void sched::yield() { alarm::ring_now(); }

void sched::sleep_for_impl(sleepqueue_clock::duration sleep_duration) {
  auto earliest_wake_time = sleepqueue_clock::now() + sleep_duration;
  auto* cur = task::current();

  lock();
  cur->run_state = task::WAITING;
  _sleepqueue.emplace(earliest_wake_time, cur);
  unlock();

  yield();
}

sched_handle sched::spawn(const attr& attr, task::entry_t* entry, void* arg) {
  sched_handle handle;

  if (branch_unexpected(entry == nullptr)) {
    throw std::domain_error("must supply entry function");
  }

  handle.t = _freelist.make_task(attr);
  handle.t->entry = entry;
  handle.t->arg = arg;
  handle.t->vruntime = _min_vruntime;

  lock();

  // this will start the task and immediately return control
  if (branch_unexpected(handle.t->start())) {
    _freelist.return_task(handle.t);
    handle.t = nullptr;
    return handle;
  }

  runqueue_push(handle.t);

  unlock();

  return handle;
}

/**
 * waits for the task indicated by |handle| to finish
 *
 * cleans up |handle|'s task's memory after it has returned
 *
 * implementation details:
 *
 * this function is very basic except for a race condition. the task's `joiner`
 * property is used as a lock to prevent the race condition from leaving
 * something possibly permanently descheduled.
 *
 * if |handle|'s task has already exited, the function runs without blocking.
 * however, if |handle|'s task is still running, the `joiner` flag is locked to
 * the current task which will deschedule itself until |handle|'s task finishes.
 */
void sched::join(sched_handle* handle, void** return_value) {
  if (branch_unexpected(handle == nullptr || !*handle)) {
    throw std::domain_error(
        "|handle| must be specified and must be a valid thread");
  }

  task* current = task::current();

  // flag to |thread| that you are the joiner
  for (task* current_joiner = nullptr; branch_unexpected(
           !handle->t->joiner.compare_exchange_strong(current_joiner, current));
       current_joiner = nullptr) {
    // if the joiner is not `nullptr` and is not locked, something else is
    // joining, which is undefined behavior
    if (branch_unexpected(current_joiner != (task*)k_pointer_lock &&
                          current_joiner != nullptr)) {
      throw std::logic_error("a thread can only be joined from one place");
    }

    yield();
  }

  // if we have locked `handle.task->joiner`, we can deschedule ourselves by
  // setting our `run_state` to "waiting" and yielding to the scheduler.
  // |handle|'s task will reschedule us when it has stopped, in which case the
  // loop will break and we know we can read off the return value and
  // (optionally) destroy the thread memory.
  while (handle->t->run_state != task::STOPPED) {
    current->run_state = task::WAITING;
    yield();
  }

  // take the return value of |thread| if |return_value| is given
  if (return_value != nullptr) {
    *return_value = handle->t->return_value;
  }

  _freelist.return_task(handle->t);
  handle->t = nullptr;
}

/**
 * immediately exits the current thread with return value |return_value|
 */
void sched::exit(void* return_value) {
  task* current = task::current();

  // indicative that the current task is the root task. abort reallly hard.
  if (branch_unexpected(current->entry == nullptr)) {
    gthread_log_fatal("cannot exit from the root task!");
  }

  // lock the joiner with a flag
  task* joiner = nullptr;
  current->joiner.compare_exchange_strong(joiner, k_pointer_lock);

  current->return_value = return_value;  // save |return_value|
  current->run_state = task::STOPPED;    // deschedule permanently

  if (joiner != nullptr) {
    // consider the very weird case where join() locked `joiner` but hasn't
    // descheduled itself yet. it would be verrrryyy bad to put something in
    // the tree twice. if this task is running and `joiner` is in the waiting
    // state, it must be descheduled.
    while (branch_unexpected(joiner->run_state != task::WAITING)) {
      yield();
    }

    lock();
    runqueue_push(joiner);
    unlock();
  } else {
    // if there wasn't a joiner that suspended itself, we entered a critical
    // section and we should unlock
    current->joiner = nullptr;
  }

  yield();  // deschedule

  // impossible to be here
  gthread_log_fatal("how did I get here?");
}

}  // namespace gthread
