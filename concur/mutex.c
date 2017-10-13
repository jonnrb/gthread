#include "mutex.h"



// Initializes a my_pthread_mutex_t created by the calling thread. Attributes
// are ignored.
int gthread_mutex_init(gthread_mutex_t* mutex,
                   	const gthread_mutexattr_t *mutexattr){
						mutex->init = 1; //initialized state
   						mutex->task = NULL;
   						mutex->state = UNLOCKED;
   						return 1;
   					}

// Locks a given mutex, other threads attempting to access this mutex will not
// run until it is unlocked.
int gthread_mutex_lock(gthread_mutex_t *mutex){
    while(gthread_cas(&(mutex->state),UNLOCKED, LOCKED) != 1){ //compare and swap mutex
   	 gthread_sched_yield();//yields if swap failed
    }
    mutex->task = gthread_task_current();
    return 1;
}

// Unlocks a given mutex.
int gthread_mutex_unlock(gthread_mutex_t *mutex){
	if(mutex -> init != 1){
		errno = EINVAL;
		perror("Invalid mutex, must initialize first.");
		return -1;
	}
    if(gthread_task_current() == mutex->task){
   	 mutex->task = NULL;
   	 return gthread_cas(&(mutex->state),LOCKED,UNLOCKED);
    }
    else{
   	 errno = EINVAL;
   	 perror("Unlock attempted by a thread/task in which mutex does not belong.")
   	 return errno;
    }
    
}

// Destroys a given mutex. Mutex should be unlocked before doing so.
int gthread_mutex_destroy(gthread_mutex_t *mutex){
	if(mutex->init == 1){
		mutex->init = 0;
		return 1;
	}
	else{
		errno = EINVAL;
		perror("Mutex was not initialized to begin with.");
		return -1;
	}
    
}

