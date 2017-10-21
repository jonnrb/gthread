/**
 * author: Khalid Akash, JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//concur/mutex.cc
 * info: mutex or binary semaphore
 */

#include "concur/mutex.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <atomic>

#include "sched/sched.h"
#include "sched/task.h"
#include "util/compiler.h"
#include "util/list.h"
#include "util/rb.h"

static inline void spin_acquire(std::atomic<bool> *lock) {
  for (bool expected = false; !lock->compare_exchange_weak(expected, true);
       expected = false) {
    gthread_sched_yield();
  }
}

// Initializes a my_pthread_mutex_t created by the calling thread. Attributes
// are ignored.
int gthread_mutex_init(gthread_mutex_t *mutex,
                       const gthread_mutexattr_t *mutexattr) {
  mutex->init = 1;  // initialized state
  mutex->owner = NULL;
  mutex->lock = false;
  mutex->priority_boost = 0;
  return 0;
}

// Locks a given mutex, other threads attempting to access this mutex will not
// run until it is unlocked.
int gthread_mutex_lock(gthread_mutex_t *mutex) {
  if (branch_unexpected(mutex->init != 1)) {
    errno = EINVAL;
    perror("Invalid mutex, must initialize first.");
    return errno;
  }

  gthread_task_t *current = gthread_task_current();

  // spin acquire lock
  spin_acquire(&mutex->lock);

  // slow path
  while (branch_unexpected(mutex->owner != NULL)) {
    ++mutex->priority_boost;
    ++mutex->owner->priority_boost;

    // put the thread on the waitqueue
    gthread_sched_uninterruptable_lock();
    current->vruntime_save = current->s.rq.vruntime;
    current->run_state = GTHREAD_TASK_WAITING;
    gthread_list_push(&mutex->waitqueue, &current->s.wq);
    gthread_sched_uninterruptable_unlock();

    mutex->lock = false;

    // deschedule self until woken up
    gthread_sched_yield();

    // reacquire mutex lock
    spin_acquire(&mutex->lock);

    --mutex->priority_boost;
  }

  mutex->owner = current;
  current->priority_boost = mutex->priority_boost;

  // release lock
  mutex->lock = 0;

  return 0;
}

// Unlocks a given mutex.
int gthread_mutex_unlock(gthread_mutex_t *mutex) {
  if (branch_unexpected(mutex->init != 1)) {
    errno = EINVAL;
    perror("Invalid mutex, must initialize first.");
    return -1;
  }

  gthread_task_t *current = gthread_task_current();
  if (branch_unexpected(current != mutex->owner)) {
    errno = EINVAL;
    perror("Unlock attempted by a thread/task in which mutex does not belong.");
    return -1;
  }

  spin_acquire(&mutex->lock);

  // fast path
  if (branch_expected(mutex->waitqueue == NULL)) {
    mutex->owner = NULL;
    mutex->lock = 0;
    return 0;
  }

  gthread_task_t *waiter =
      container_of(gthread_list_pop(&mutex->waitqueue), gthread_task_t, s.wq);
  gthread_rb_construct(&waiter->s.rq.rb_node);
  waiter->s.rq.vruntime = waiter->vruntime_save;
  waiter->run_state = GTHREAD_TASK_SUSPENDED;

  gthread_sched_uninterruptable_lock();
  extern gthread_rb_tree_t gthread_sched_runqueue;
  gthread_rb_push(&gthread_sched_runqueue, &waiter->s.rq.rb_node);
  gthread_sched_uninterruptable_unlock();

  mutex->owner = NULL;
  current->priority_boost = 0;
  mutex->lock = false;
  gthread_sched_yield();

  return 0;
}

// Destroys a given mutex. Mutex should be unlocked before doing so.
int gthread_mutex_destroy(gthread_mutex_t *mutex) {
  if (branch_unexpected(mutex->init == 1)) {
    mutex->init = 0;
    mutex->lock = false;
    mutex->owner = nullptr;
    return 0;
  } else {
    errno = EINVAL;
    perror("Mutex was not initialized to begin with");
    return -1;
  }
}
