#ifndef CONCUR_MUTEX_H_
#define CONCUR_MUTEX_H_
#include "sched/sched.h"
#include "sched/task.h"
#include "arch/atomic.h"
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>


typedef enum{
    LOCKED,
    UNLOCKED
}lock;

typedef struct gthread_mutex_data{
	int init;
    lock state;
    gthread_task_t* task;
    // char which_task; //test purposes
}gthread_mutex_t;

typedef struct mutexattr{
//dummy struct to allow compilation
}gthread_mutexattr_t;


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
