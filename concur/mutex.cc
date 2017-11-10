#include "concur/mutex.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <atomic>
#include <mutex>

#include "sched/sched.h"
#include "sched/task.h"
#include "util/compiler.h"

namespace gthread {

namespace {
static inline void spin_acquire(std::atomic_flag* lock) {
  while (!lock->test_and_set()) sched::get().yield();
}
}  // namespace

mutex::mutex(const mutexattr& a)
    : _lock{false}, _owner{nullptr}, _priority_boost{0} {}

mutex::~mutex() {}

using sched_lock = std::lock_guard<sched>;

// locks the mutex. will wait if the lock is contended.
int mutex::lock() {
  task* cur = task::current();

  // spin acquire lock
  spin_acquire(&_lock);

  // slow path
  while (branch_unexpected(_owner != nullptr)) {
    ++_priority_boost;
    ++_owner->priority_boost;

    // put the thread on the waitqueue
    {
      sched_lock l(sched::get());
      cur->run_state = task::WAITING;
      _waitqueue.push_back(cur);
    }

    _lock.clear();

    // deschedule self until woken up
    sched::get().yield();

    // reacquire mutex lock
    spin_acquire(&_lock);

    --_priority_boost;
  }

  _owner = cur;
  cur->priority_boost = _priority_boost;

  // release lock
  _lock.clear();

  return 0;
}

// Unlocks a given mutex.
int mutex::unlock() {
  task* cur = task::current();
  if (branch_unexpected(_owner != cur)) {
    errno = EINVAL;
    return -1;
  }

  spin_acquire(&_lock);

  // fast path
  if (branch_expected(_waitqueue.empty())) {
    _owner = nullptr;
    _lock.clear();
    return 0;
  }

  task* waiter = *_waitqueue.begin();
  _waitqueue.pop_front();

  waiter->run_state = task::SUSPENDED;

  {
    sched_lock l(sched::get());
    sched::get().runqueue_push(waiter);
  }

  _owner = nullptr;
  cur->priority_boost = 0;

  _lock.clear();

  sched::get().yield();

  return 0;
}

}  // namespace gthread
