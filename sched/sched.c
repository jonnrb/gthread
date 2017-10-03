/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//sched/sched.c
 * info: scheduler for uniprocessors
 */

#include "sched/sched.h"

#include <errno.h>

#include "arch/atomic.h"

static uint64_t g_is_sched_init = 0;

#define k_num_tasks 1024

// g_root_task is the initial task
static gthread_task_t* g_root_task = NULL;

// g_tasks[0] is the scheduler task
static gthread_task_t g_tasks[k_num_tasks] = {{0}};

/**
 * basic round robin support. iterates through the global tasks and finds the
 * next one that is suspended and returns it.
 */
static gthread_task_t* next_task_round_robin() {
  static int i = 0;

  gthread_task_t* task;
  do {
    task = (i == 0) ? g_root_task : &g_tasks[i];
    i = (i + 1) % k_num_tasks;
  } while (task->run_state != GTHREAD_TASK_SUSPENDED);

  return task;
}

static gthread_task_t* make_task(gthread_attr_t* attr) {
  for (int i = 1; i < k_num_tasks; ++i) {
    if (g_tasks[i].run_state == GTHREAD_TASK_FREE &&
        gthread_cas(&g_tasks[i].run_state, GTHREAD_TASK_FREE,
                    GTHREAD_TASK_LOCKED)) {
      if (gthread_construct_task(&g_tasks[i], attr)) return NULL;
      return &g_tasks[i];
    }
  }

  return NULL;
}

// on preemption, switch to scheduler.
static gthread_task_t* sched_timer(gthread_task_t* task) { return &g_tasks[0]; }

// quite poor scheduler implementation: basic round robin.
static void* sched_main(void* _) {
  gthread_task_set_time_slice_trap(sched_timer, 50 * 1000);

  for (;;) {
    // TODO: remove these asserts
    gthread_task_t* task = next_task_round_robin();
    assert(task != NULL);
    assert(!gthread_switch_to_task(task));
  }
}

int gthread_sched_init() {
  if (!gthread_cas(&g_is_sched_init, 0, 1)) return -1;

  g_root_task = gthread_get_current_task();

  gthread_attr_t* sched_attr = NULL;  // TODO: probably should have small stack.
  assert(!gthread_construct_task(&g_tasks[0], sched_attr));
  assert(!gthread_start_task(&g_tasks[0], sched_main, NULL));

  return 0;
}

int gthread_sched_yield() {
  if (!g_is_sched_init) return -1;
  gthread_switch_to_task(&g_tasks[0]);
  return 0;
}

int gthread_sched_spawn(gthread_sched_handle_t* handle, gthread_attr_t* attr,
                        gthread_entry_t* entry, void* arg) {
  if (handle == NULL || entry == NULL) {
    errno = EINVAL;
    return -1;
  }

  gthread_task_t* task = make_task(attr);

  if (gthread_start_task(task, entry, arg)) {
    return -1;
  }

  *handle = task;

  return 0;
}
