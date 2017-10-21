/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//sched/task.c
 * info: generic task switching functions for a scheduler
 */

#include "sched/task.h"

#include "arch/atomic.h"
#include "gthread.h"
#include "platform/memory.h"
#include "util/log.h"
#include "util/rb.h"

// task representing the initial context of execution.
static gthread_task_t g_root_task = {0};

static gthread_task_end_handler_t* g_task_end_handler = NULL;

// task switching MUST not be reentrant.
static uint64_t g_lock = 0;

static int g_timer_enabled = 0;
static gthread_task_time_slice_trap_t* g_time_slice_trap = NULL;

uint64_t gthread_task_is_root_task_init = false;

void gthread_task_module_init();

gthread_task_t* gthread_task_construct(gthread_attr_t* attrs) {
  gthread_tls_t tls = gthread_tls_allocate();
  if (tls == NULL) return NULL;

  gthread_task_t* task = gthread_allocate(sizeof(gthread_task_t));
  if (task == NULL) {
    gthread_tls_free(tls);
    return NULL;
  }
  gthread_tls_set_thread(tls, task);
  task->tls = tls;

  task->run_state = GTHREAD_TASK_STOPPED;
  task->joiner = NULL;
  task->return_value = NULL;
  if (gthread_allocate_stack(attrs, &task->stack, &task->total_stack_size)) {
    gthread_tls_free(tls);
    gthread_free(task);
    return NULL;
  }

  // prep for runqueue
  gthread_rb_construct(&task->s.rq.rb_node);
  task->s.rq.vruntime = 0;
  task->vruntime_save = 0;
  task->priority_boost = 0;

  return task;
}

static inline void record_time_slice(gthread_task_t* task, uint64_t elapsed) {
  if (task->priority_boost > 0) {
    elapsed /= (task->priority_boost + 1);
  }

  // XXX hack to hurt spinlocks
  if (elapsed < 10) elapsed = 10;

  task->s.rq.vruntime += elapsed;
}

static inline void reset_timer_and_record_time(gthread_task_t* task) {
  if (g_timer_enabled) {
    uint64_t elapsed = gthread_timer_reset();
    record_time_slice(task, elapsed);
  }
}

// root of task stack trace! :)
static void gthread_task_entry(void* arg) {
  // swtich back to the starter to clean up
  gthread_saved_ctx_t** from_and_to = (gthread_saved_ctx_t**)arg;
  gthread_switch_to(from_and_to[0], from_and_to[1]);

  // when we are here, tls should be set up
  gthread_task_t* current = gthread_task_current();
  current->run_state = GTHREAD_TASK_RUNNING;
  current->return_value = current->entry(current->arg);
  g_task_end_handler(current);
}

int gthread_task_start(gthread_task_t* task) {
  gthread_task_t* current = gthread_task_current();

  if (branch_unexpected(task == current)) {
    gthread_log_fatal("cannot start current task from the current task!");
  }

  // slow path on being the first call in this module
  if (branch_unexpected(!gthread_task_is_root_task_init)) {
    gthread_task_module_init();
  }

  gthread_saved_ctx_t* new_and_cur[2] = {&task->ctx, &current->ctx};
  gthread_switch_to_and_spawn(&current->ctx, &current->ctx, task->stack,
                              gthread_task_entry, new_and_cur);
  task->run_state = GTHREAD_TASK_SUSPENDED;

  return 0;
}

int gthread_task_reset(gthread_task_t* task) {
  if (gthread_tls_reset(task->tls)) return -1;
  gthread_rb_construct(&task->s.rq.rb_node);
  task->run_state = GTHREAD_TASK_STOPPED;
  task->return_value = NULL;
  task->joiner = NULL;
  task->s.rq.vruntime = 0;
  task->vruntime_save = 0;
  task->priority_boost = 0;

  return 0;
}

void gthread_task_destruct(gthread_task_t* task) {
  gthread_tls_free(task->tls);
  gthread_free_stack((char*)task->stack - task->total_stack_size,
                     task->total_stack_size);
  gthread_free(task);
}

static int switch_to_task(gthread_task_t* task, uint64_t* elapsed) {
  gthread_task_t* prev_task = gthread_task_current();
  if (task == prev_task) {
    assert(0);  // DO NOT SUBMIT: test for now
    return 0;
  }

  // slow path on being the first call in this module
  if (branch_unexpected(!gthread_task_is_root_task_init)) {
    gthread_task_module_init();
  }

  if (!gthread_cas(&g_lock, 0, 1)) return -1;

  // if the task sets its own `run_state`, respect that value
  if (prev_task->run_state == GTHREAD_TASK_RUNNING) {
    prev_task->run_state = GTHREAD_TASK_SUSPENDED;
  }

  if (elapsed == NULL) {
    reset_timer_and_record_time(prev_task);
  } else {
    record_time_slice(prev_task, *elapsed);
  }

  // officially in the |task|'s context
  gthread_tls_use(task->tls);

  g_lock = 0;
  gthread_switch_to(&prev_task->ctx, &task->ctx);

  assert(gthread_task_current() == prev_task);
  prev_task->run_state = GTHREAD_TASK_RUNNING;
  return 0;
}

int gthread_task_switch_to(gthread_task_t* task) {
  return switch_to_task(task, NULL);
}

static void time_slice_trap(uint64_t elapsed) {
  gthread_task_t* current = gthread_task_current();
  gthread_task_t* next_task = NULL;

  if (g_time_slice_trap != NULL) {
    next_task = g_time_slice_trap(current);
  }

  if (next_task != NULL && next_task != current) {
    switch_to_task(next_task, &elapsed);
  } else {
    record_time_slice(current, elapsed);
  }
}

int gthread_task_set_time_slice_trap(gthread_task_time_slice_trap_t* trap,
                                     uint64_t usec) {
  g_timer_enabled = usec != 0;
  g_time_slice_trap = trap;
  gthread_timer_set_trap(time_slice_trap);
  return gthread_timer_set_interval(usec);
}

void gthread_task_set_end_handler(gthread_task_end_handler_t handler) {
  g_task_end_handler = handler;
}

static void default_task_end_handler(gthread_task_t* _) {
  gthread_log_fatal("no task end handler set!");
}

void gthread_task_module_init() {
  if (branch_unexpected(
          !gthread_task_is_root_task_init &&
          gthread_cas(&gthread_task_is_root_task_init, false, true))) {
    // most of struct is zero-initialized
    g_root_task.tls = gthread_tls_current();
    g_root_task.run_state = GTHREAD_TASK_RUNNING;
    gthread_rb_construct(&g_root_task.s.rq.rb_node);

    gthread_tls_set_thread(g_root_task.tls, &g_root_task);

    if (g_task_end_handler == NULL) {
      gthread_task_set_end_handler(default_task_end_handler);
    }
  }
}
