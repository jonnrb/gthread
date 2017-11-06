/**
 * author: Khalid Akash, JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//concur/mutex_test.c
 * info: tests mutex by locking and unlocking in a tight loop across threads
 */

#include "concur/mutex.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "platform/clock.h"
#include "sched/sched.h"
#include "mymalloc.h"



void* important_task(void* arg) {
  const char* msg = (const char*)arg;

  for (int i = 0; i < 26; ++i) {
	  void* p = mymalloc(i+1, (gthread_task_t*)gthread_tls_current_thread());
	  myfree(p, (gthread_task_t*)gthread_tls_current_thread());
  }
printpagemem();
  return NULL;
}

int init() {
  gthread_sched_handle_t tasks[26];
  char msgs[26] = {'A'};
  for (int i = 0; i < 26; ++i) {
    printf("creating task %d\n", i);
    msgs[i] = msgs[0] + i;
    assert(!gthread_sched_spawn(&tasks[i], NULL, important_task,
                                (void*)(&msgs[i])));
  }


  for (int i = 0; i < 26; ++i) {
    gthread_sched_join(tasks[i], NULL);
  }
  printf("done and stuff\n");
  return 0;
}

int main() {
  init();
  return 0;
}
