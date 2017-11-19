/**
 * author: Khalid Akash, JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//concur/mutex_test.c
 * info: tests mutex by locking and unlocking in a tight loop across threads
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "platform/clock.h"
#include "sched/sched.h"
#include "mymalloc.h"

void* tasksupersimple(void* _) {
  printf("TASK SUPER SIMPLE\n");

  printf("TASK SUPER SIMPLE: allocating\n");
  volatile int* p = (int*)mymalloc(1000 * sizeof(int), gthread_task_current());

  printf("TASK SUPER SIMPLE: writing\n");
  for (int i = 0; i < 1000; ++i) {
    p[i] = i;
  }

  printf("TASK SUPER SIMPLE: freeing\n");
  myfree((void*)p, gthread_task_current());

  return NULL;
}

void* task1(void* arg) {
  printf("TASK1\n");

  void* p = mymalloc(10000, gthread_task_current());
  void* x = mymalloc(20000, gthread_task_current());
  void* z = mymalloc(10000, gthread_task_current());
  myfree(p, gthread_task_current());
  myfree(x, gthread_task_current());
  myfree(z, gthread_task_current());

  printInternalMemory(gthread_task_current());

  void* sp = shalloc(1000);
  myfree(sp, gthread_task_current());
  printShallocRegion();

  return NULL;
}

void* task2(void* arg) {
  printf("TASK2\n");

  void* p = mymalloc(8000000, gthread_task_current());
  void* x = mymalloc(8000000, gthread_task_current());
  //printThread(gthread_task_current());
  printInternalMemory(gthread_task_current());

  shalloc(2000);
  printShallocRegion();
  return NULL;
}

void* task3(void* arg){
  printf("TASK3\n");

  void* p = mymalloc(8000000, gthread_task_current());
  printInternalMemory(gthread_task_current());

  shalloc(3000);
  printShallocRegion();

  return NULL;
}

int main() {
  int ret;

  gthread_sched_handle_t simple_task = NULL;
  ret = gthread_sched_spawn(&simple_task, NULL, tasksupersimple, NULL);
  assert(!ret);
  ret = gthread_sched_join(simple_task, NULL);
  assert(!ret);

  return 0;

  gthread_sched_handle_t parallel_tasks[3] = {NULL};

  ret = gthread_sched_spawn(&parallel_tasks[0], NULL, task1, NULL);
  assert(!ret);
  ret = gthread_sched_spawn(&parallel_tasks[1], NULL, task2, NULL);
  assert(!ret);
  ret = gthread_sched_spawn(&parallel_tasks[2], NULL, task3, NULL);
  assert(!ret);

  for (int i = 0; i < 3; ++i) {
    if (parallel_tasks[i] != NULL) {
      ret = gthread_sched_join(parallel_tasks[i], NULL);
      assert(!ret);
    }
  }

  printf("Test finished\n");
  return 0;
}
