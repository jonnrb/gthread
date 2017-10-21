/**
 * author: Khalid Akash, JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//concur/mutex.h
 * info: mutex or binary semaphore
 */

#ifndef CONCUR_MUTEX_H_
#define CONCUR_MUTEX_H_

#include <atomic>

#include "sched/task.h"
#include "util/list.h"

typedef struct gthread_mutex_data {
  int init;
  std::atomic<bool> lock;
  gthread_task_t *owner;
  gthread_list_t waitqueue;
  uint64_t priority_boost;
} gthread_mutex_t;

typedef struct mutexattr {
  // dummy struct to allow compilation
} gthread_mutexattr_t;

// Initializes a my_pthread_mutex_t created by the calling thread. Attributes
// are ignored.
int gthread_mutex_init(gthread_mutex_t *mutex,
                       const gthread_mutexattr_t *mutexattr);

// Locks a given mutex, other threads attempting to access this mutex will not
// run until it is unlocked.
int gthread_mutex_lock(gthread_mutex_t *mutex);

// Unlocks a given mutex.
int gthread_mutex_unlock(gthread_mutex_t *mutex);

// Destroys a given mutex. Mutex should be unlocked before doing so.
int gthread_mutex_destroy(gthread_mutex_t *mutex);

#endif  // CONCUR_MUTEX_H_
