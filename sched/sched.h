/**
 * author: JonNRb <jonbetti@gmail.com>, Matthew Handzy <matthewhandzy@gmail.com>
 * license: MIT
 * file: @gthread//sched/sched.h
 * info: scheduler for uniprocessors
 */

#ifndef SCHED_SCHED_H_
#define SCHED_SCHED_H_

#include "gthread.h"
#include "sched/task.h"

/**
 * a handle to a currently running thread
 *
 * example: ```
 *   gthread_sched_handle_t task;                          // declares handle
 *   gthread_sched_spawn(&task, NULL, my_function, NULL);  // spawns a task
 *   gthread_sched_join(task, NULL);                       // joins the task
 * ```
 */
typedef gthread_task_t* gthread_sched_handle_t;

/**
 * stops the current task and invokes the scheduler
 *
 * the scheduler will pick the task that has had the minimum amount of
 * proportional processor time and run that task. when the current task meets
 * that condition, it will be resume on the line following this call.
 */
int gthread_sched_yield();

/**
 * spawns a gthread, storing a handle in |handle|, where |entry| will be
 * invoked with argument |arg|. `gthread_sched_join()` must be called
 * eventually to clean up the called thread when it finishes
 *
 * TODO: gthread_sched_detach()
 */
int gthread_sched_spawn(gthread_sched_handle_t* handle, gthread_attr_t* attr,
                        gthread_entry_t entry, void* arg);

/**
 * disables preemption by the scheduler
 *
 * helper function for things that implement cooperative functionality
 * to make themselves uninterruptable for the *only* purpose of marking the
 * current task as waiting and putting it on a waitqueue of some kind
 *
 * this *disables* the scheduler in the sense that when the scheduler is
 * invoked, the locking task will be resumed
 */
static inline void gthread_sched_uninterruptable_lock();

/**
 * resumes preemption by the scheduler if it has been disabled
 *
 * if `gthread_sched_uninterruptable_lock()` is called, this must be called to
 * allow preemption to happen again
 */
static inline void gthread_sched_uninterruptable_unlock();

/**
 * joins |task| when it finishes running (the caller will be suspended)
 *
 * |return_value| can be NULL. when |return_value| is not NULL, `*return_value`
 * will contain the return value of |task|, either from the returning entry
 * point, or passed in via `gthread_sched_exit()`.
 */
int gthread_sched_join(gthread_sched_handle_t task, void** return_value);

/**
 * ends the current task with return value |return_value|
 *
 * the current task normally ends by returning a value from its entry point.
 * however, if it must end abruptly, this function must be used.
 */
void gthread_sched_exit(void* return_value);

#include "sched/sched_inline.h"

#endif  // SCHED_SCHED_H_
