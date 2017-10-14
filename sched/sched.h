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

typedef gthread_task_t* gthread_sched_handle_t;

int gthread_sched_init();

int gthread_sched_yield();

int gthread_sched_spawn(gthread_sched_handle_t* handle, gthread_attr_t* attr,
                        gthread_entry_t* entry, void* arg);

int gthread_sched_join(gthread_sched_handle_t thread, void** return_value);

void gthread_sched_exit(void* return_value);

// gthread_t gthread_sched_self();

#endif  // SCHED_SCHED_H_
