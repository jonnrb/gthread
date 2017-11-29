#include "concur/mutex.h"

#include <atomic>
#include <cassert>
#include <mutex>
#include <stdexcept>

#include "gthread.h"
#include "sched/task.h"
#include "util/compiler.h"

namespace gthread {
mutex::mutex(const mutexattr& a)
    : _lock(), _owner(nullptr), _waitqueue(), _priority_boost(0) {}

mutex::~mutex() {}

// locks the mutex. will wait if the lock is contended.
int mutex::lock() {
  std::unique_lock<internal::spin_lock> l(_lock);

  // slow path
  while (branch_unexpected(_owner != nullptr)) {
    ++_priority_boost;
    ++_owner->priority_boost;

    // put the thread on the waitqueue
    auto& waiter = _waitqueue.emplace_back();
    l.unlock();
    bool succ = waiter.park();
    assert(succ);
    l.lock();

    --_priority_boost;
  }

  auto* cur = task::current();
  _owner = cur;
  cur->priority_boost = _priority_boost;

  return 0;
}

// Unlocks a given mutex.
int mutex::unlock() {
  auto* current = task::current();

  std::unique_lock<internal::spin_lock> l(_lock);

  if (branch_unexpected(_owner != current)) {
    throw std::logic_error("unlock called from context other than the owner's");
  }

  // fast path
  if (branch_expected(_waitqueue.empty())) {
    _owner = nullptr;
    l.unlock();
    return 0;
  }

  auto waiter = _waitqueue.begin();
  while (!waiter->unpark()) self::yield();
  _waitqueue.pop_front();
  _owner = nullptr;
  current->priority_boost = 0;

  // unlock spinlock before yielding
  l.unlock();
  self::yield();
  return 0;
}

}  // namespace gthread
