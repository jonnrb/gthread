/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//sched/sched.c
 * info: scheduler for uniprocessors
 */

#include "sched/sched.h"

#include <errno.h>
#include <stdio.h>

#include "arch/atomic.h"

static uint64_t g_is_sched_init = 0;

#define k_num_tasks 1024

// g_tasks[0] is the initial task
static gthread_task_t* g_tasks[k_num_tasks] = {NULL};

/**
 * basic round robin support. iterates through the global tasks and finds the
 * next one that is suspended and returns it.
 */
static gthread_task_t* next_task_round_robin() {
  static int i = 0;

  gthread_task_t* task;
  do {
    task = g_tasks[i];
    i = (i + 1) % k_num_tasks;
  } while (task == NULL || task->run_state != GTHREAD_TASK_SUSPENDED);

  return task;
}

static gthread_task_t* make_task(gthread_attr_t* attr) {
  // start at 1, main task will never be usable
  for (int i = 1; i < k_num_tasks; ++i) {
    if (g_tasks[i] != NULL && g_tasks[i]->run_state == GTHREAD_TASK_FREE &&
        gthread_cas(&g_tasks[i]->run_state, GTHREAD_TASK_FREE,
                    GTHREAD_TASK_LOCKED)) {
      gthread_task_reset(g_tasks[i]);
      return g_tasks[i];
    }
  }

  gthread_task_t* task = gthread_task_construct(attr);

  if (task == NULL) {
    perror("task construction failed");
    return NULL;
  }

  for (int i = 1; i < k_num_tasks; ++i) {
    if (g_tasks[i] == NULL &&
        gthread_cas((uint64_t*)&g_tasks[i], (uint64_t)NULL, (uint64_t)task)) {
      return task;
    }
  }

  // no way to destruct tasks right now. abort.
  assert(0);

  return NULL;
}

// quite poor scheduler implementation: basic round robin.
static gthread_task_t* sched_timer(gthread_task_t* task) {
  return next_task_round_robin();
}

int gthread_sched_init() {
  if (!gthread_cas(&g_is_sched_init, 0, 1)) return -1;

  gthread_task_set_time_slice_trap(sched_timer, 50 * 1000);

  g_tasks[0] = gthread_task_current();
  assert(g_tasks[0] != NULL);

  return 0;
}

int gthread_sched_yield() {
  if (!g_is_sched_init) return -1;
  return gthread_timer_alarm_now();
}

int gthread_sched_spawn(gthread_sched_handle_t* handle, gthread_attr_t* attr,
                        gthread_entry_t* entry, void* arg) {
  if (handle == NULL || entry == NULL) {
    errno = EINVAL;
    return -1;
  }

  gthread_task_t* task = make_task(attr);

  if (gthread_task_start(task, entry, arg)) {
    return -1;
  }

  *handle = task;

  return 0;
}
