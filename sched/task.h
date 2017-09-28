/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//sched/task.h
 * info: generic task switching functions for a scheduler
 */

#ifndef SCHED_TASK_H_
#define SCHED_TASK_H_

#include <stdlib.h>

#include "arch/switch_to.h"
#include "gthread.h"
#include "platform/timer.h"

typedef enum {
  GTHREAD_TASK_FREE = 0,
  GTHREAD_TASK_RUNNING = 1,
  GTHREAD_TASK_SUSPENDED = 2,
  GTHREAD_TASK_STOPPED = 3,
  GTHREAD_TASK_LOCKED = 4
} gthread_task_run_state_t;

typedef struct _task {
  uint64_t run_state;
  void* return_value;

  gthread_saved_ctx_t ctx;
  void* stack;
  size_t total_stack_size;

  uint64_t slice_times[16];
  int slice_i;
} gthread_task_t;

// constructs a stack and sets |task| to default values.
int gthread_construct_task(gthread_task_t* task, gthread_attr_t* attrs);

int gthread_start_task(gthread_task_t* task, gthread_entry_t* entry, void* arg);

// int gthread_stop_task(gthread_task_t* task, void* ret_val);

// int gthread_reap_task(gthread_task_t* task);

gthread_task_t* gthread_get_current_task();

// suspends the currently running task and switches to |task|.
int gthread_switch_to_task(gthread_task_t* task);

typedef gthread_task_t* gthread_task_time_slice_trap_t(gthread_task_t*);

int gthread_task_set_time_slice_trap(gthread_task_time_slice_trap_t* trap,
                                     uint64_t usec);

typedef void gthread_task_end_handler_t(gthread_task_t*);

int gthread_set_task_end_handler(gthread_task_end_handler_t* task_end_handler);

#endif  // SCHED_TASK_H_
