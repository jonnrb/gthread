/**
 * author: JonNRb <jonbetti@gmail.com>,
 *         Matthew Handzy <matthewhandzy@gmail.com>, Khalid Akash
 * license: MIT
 * file: @gthread//my_pthread.h
 * info: wrapper functions for project spec
 */

#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H

// have to use relative includes since
#include "concur/mutex.h"
#include "gthread.h"
#include "sched/sched.h"
#include "sched/task.h"

// ships with macro mappings since the benchmark is ALWAYS compiled with
// -pthread
#define USE_MY_PTHREAD 1

#ifdef USE_MY_PTHREAD
#define pthread_attr_t gthread_attr_t
#define pthread_t my_pthread_t
#define pthread_mutex_t my_pthread_mutex_t
#define pthread_mutex_attr_t void
#define pthread_create my_pthread_create
#define pthread_exit my_pthread_exit
#define pthread_join my_pthread_join
#define pthread_mutex_init my_pthread_mutex_init
#define pthread_mutex_lock my_pthread_mutex_lock
#define pthread_mutex_unlock my_pthread_mutex_unlock
#define pthread_mutex_destroy my_pthread_mutex_destroy
#endif  // USE_MY_PTHREAD

typedef gthread_sched_handle_t my_pthread_t;

typedef gthread_mutex_t my_pthread_mutex_t;

static inline int my_pthread_create(my_pthread_t* thread, pthread_attr_t* attr,
                                    void* (*function)(void*), void* arg) {
  // |attr| is NULL by spec
  return gthread_sched_spawn(thread, NULL, function, arg);
}

static inline int my_pthread_yield() { gthread_sched_yield(); }

static inline void my_pthread_exit(void* value_ptr) {
  gthread_sched_exit(value_ptr);
}

static inline int my_pthread_join(my_pthread_t thread, void** value_ptr) {
  return gthread_sched_join(thread, value_ptr);
}

static inline int my_pthread_mutex_init(my_pthread_mutex_t* mutex,
                                        const pthread_mutexattr_t* mutexattr) {
  // |attr| is NULL by spec
  gthread_mutex_init(mutex, NULL);
}

static inline int my_pthread_mutex_lock(my_pthread_mutex_t* mutex) {
  return gthread_mutex_lock(mutex);
}

static inline int my_pthread_mutex_unlock(my_pthread_mutex_t* mutex) {
  return gthread_mutex_unlock(mutex);
}

static inline int my_pthread_mutex_destroy(my_pthread_mutex_t* mutex) {
  return gthread_mutex_destroy(mutex);
}

#endif  // MY_PTHREAD_T_H
