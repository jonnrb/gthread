/**
 * author: JonNRb <jonbetti@gmail.com>, Matthew Handzy <matthewhandzy@gmail.com>
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
#include "platform/tls.h"
#include "util/compiler.h"
#include "util/rb.h"

typedef enum {
  GTHREAD_TASK_FREE = 0,
  GTHREAD_TASK_RUNNING = 1,
  GTHREAD_TASK_SUSPENDED = 2,
  GTHREAD_TASK_STOPPED = 3,
  GTHREAD_TASK_WAITING = 4
} gthread_task_run_state_t;

typedef struct gthread_task {
  gthread_tls_t tls;

  struct gthread_task* joiner;
  uint64_t run_state;
  void* return_value;

  gthread_saved_ctx_t ctx;
  void* stack;
  size_t total_stack_size;

  gthread_rb_node_t rb_node;
  uint64_t vruntime;       // microseconds
  uint64_t vruntime_save;  // used to save `vruntime` when adding the task to a
                           // waitqueue
} gthread_task_t;

// constructs a stack and sets |task| to default values.
gthread_task_t* gthread_task_construct(gthread_attr_t* attrs);

int gthread_task_start(gthread_task_t* task, gthread_entry_t* entry, void* arg);

int gthread_task_reset(gthread_task_t* task);

void gthread_task_destruct(gthread_task_t* task);

static inline gthread_task_t* gthread_task_current();

// suspends the currently running task and switches to |task|.
int gthread_switch_to_task(gthread_task_t* task);

typedef gthread_task_t* gthread_task_time_slice_trap_t(gthread_task_t*);

int gthread_task_set_time_slice_trap(gthread_task_time_slice_trap_t* trap,
                                     uint64_t usec);

typedef void gthread_task_end_handler_t(gthread_task_t*);

int gthread_task_set_end_handler(gthread_task_end_handler_t* task_end_handler);

void gthread_task_module_init();

#include "sched/task_inline.h"

#endif  // SCHED_TASK_H_
