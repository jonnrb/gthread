#include "sched/sched.h"

#include <atomic>
#include <cassert>
#include <iostream>
#include <mutex>
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

sched::sched() : _interrupt_lock(UNLOCKED), _min_vruntime(0), _freelist(64) {
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
  // don't switch tasks
  auto expected = UNLOCKED;
  if (branch_unexpected(!_interrupt_lock.compare_exchange_strong(
          expected, LOCKED_IN_SCHED, std::memory_order_acquire))) {
    if (expected != LOCKED_IN_TASK) {
      gthread_log_fatal(
          "scheduler was interrupted by itself! this should be impossible. "
          "bug???");
    }
    return nullptr;
  }

  // if the task was in a runnable state when the scheduler was invoked, push it
  // to the runqueue
  if (last_running_task->run_state == task::RUNNING) {
    runqueue_push(last_running_task);
  } else if (last_running_task->run_state == task::STOPPED &&
             last_running_task->detached) {
    _freelist.return_task<true>(last_running_task);
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
      auto time = sleeper->first;
      auto* t = sleeper->second;
      _sleepqueue.erase(sleeper);
      std::this_thread::sleep_until(time);
      _runqueue.emplace(t);
    }
  } else if (_runqueue.empty()) {
    gthread_log_fatal("nothing to do. deadlock?");
  }

  auto begin = _runqueue.begin();
  task* next_task = *begin;
  _runqueue.erase(begin);

  _interrupt_lock.store(UNLOCKED, std::memory_order_release);

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

  {
    std::lock_guard l(*this);
    // this will start the task and immediately return control
    if (branch_expected(!handle.t->start())) {
      runqueue_push(handle.t);
      return handle;
    }
  }

  _freelist.return_task(handle.t);
  handle.t = nullptr;
  return handle;
}

void sched::join(sched_handle* handle, void** return_value) {
  if (branch_unexpected(handle == nullptr || !*handle)) {
    throw std::domain_error(
        "|handle| must be specified and must be a valid thread");
  }

  auto* current = task::current();

  {
    std::unique_lock l(*this);

    // if the joiner is not `nullptr`, something else is joining, which is
    // undefined behavior
    if (branch_unexpected(handle->t->joiner != nullptr)) {
      throw std::logic_error("a thread can only be joined from one place");
    }
    if (handle->t->run_state != task::STOPPED) {
      handle->t->joiner = current;
      current->run_state = task::WAITING;
      l.unlock();
      yield();
    }
  }

  assert(handle->t->run_state == task::STOPPED);

  // take the return value of |thread| if |return_value| is given
  if (return_value != nullptr) {
    *return_value = handle->t->return_value;
  }

  _freelist.return_task(handle->t);
  handle->t = nullptr;
}

void sched::detach(sched_handle* handle) {
  if (branch_unexpected(handle == nullptr || !*handle)) {
    throw std::domain_error(
        "|handle| must be specified and must be a valid thread");
  }

  {
    std::lock_guard l(*this);
    if (handle->t->run_state != task::STOPPED) {
      handle->t->detached = true;
      handle->t = nullptr;
      return;
    }
  }

  assert(handle->t->run_state == task::STOPPED);

  _freelist.return_task(handle->t);
  handle->t = nullptr;
}

void sched::exit(void* return_value) {
  auto* current = task::current();

  // indicative that the current task is the root task. abort reallly hard.
  if (branch_unexpected(current->entry == nullptr)) {
    gthread_log_fatal("cannot exit from the root task!");
  }

  {
    std::lock_guard l(*this);
    current->return_value = return_value;  // save |return_value|
    current->run_state = task::STOPPED;    // deschedule permanently

    if (current->joiner != nullptr) {
      runqueue_push(current->joiner);
    }
  }

  yield();  // deschedule

  // impossible to be here
  gthread_log_fatal("how did I get here?");
}
}  // namespace gthread
