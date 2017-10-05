#ifndef CONCUR_MUTEX_H_
#define CONCUR_MUTEX_H_

/**
 * "I've often joked that instead of picking up Djikstra's cute acronym we
 * should have called the basic synchronization object "the bottleneck".
 * Bottlenecks are useful at times, sometimes indispensible -- but they're
 * never GOOD. At best they're a necessary evil. Anything. ANYTHING that
 * encourages anyone to overuse them, to hold them too long, is bad. It's
 * NOT just the straightline performance impact of the extra recursion
 * logic in the mutex lock and unlock that's the issue here -- it's the far
 * greater, wider, and much more difficult to characterize impact on the
 * overall application concurrency."
 *
 * -- David Butenhof (the guy who literally wrote THE book on POSIX threads)
 *
 * http://www.zaval.org/resources/library/butenhof1.html
 */

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
