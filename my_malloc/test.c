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



void* important_task(void* arg) {
  const char* msg = (const char*)arg;
   void* p;
   void* x;
   void* z;

 		  p = (int*)mymalloc(10000, (gthread_task_t*)gthread_tls_current_thread());
 		  x = (int*)mymalloc(20000, (gthread_task_t*)gthread_tls_current_thread());
 		  z = (int*)mymalloc(10000, (gthread_task_t*)gthread_tls_current_thread());


 		 // debug("FREEFREEFREEFREEFREEFREEFREEFREEFREEFREEFREEFREEFREEFREEFREEFREEFREEFREEFREEFREEFREEFREEFREEFREE");
 		  myfree(x, (gthread_task_t*)gthread_tls_current_thread());
 		x = (int*)mymalloc(150, (gthread_task_t*)gthread_tls_current_thread());
 		// printInternalMemory((gthread_task_t*)gthread_tls_current_thread());
 		// x = (int*)mymalloc(10000, (gthread_task_t*)gthread_tls_current_thread());
 		// printInternalMemory((gthread_task_t*)gthread_tls_current_thread());
 		// printThread((gthread_task_t*)gthread_tls_current_thread());
 		  //myfree(z, (gthread_task_t*)gthread_tls_current_thread());
 		  //myfree(p, (gthread_task_t*)gthread_tls_current_thread());




  return NULL;
}

int init() {
  gthread_sched_handle_t tasks[26];
  char msgs[26] = {'A'};
 /* for (int i = 0; i < 26; ++i) {
    printf("creating task %d\n", i);
    msgs[i] = msgs[0] + i;
    assert(!gthread_sched_spawn(&tasks[i], NULL, important_task,
                                (void*)(&msgs[i])));
  }*/
  assert(!gthread_sched_spawn(&tasks[0], NULL, important_task,
                               (void*)(&msgs[0])));
  printf("=======================================================\n");
  assert(!gthread_sched_spawn(&tasks[1], NULL, important_task,
                                (void*)(&msgs[0])));

    gthread_sched_join(tasks[0], NULL);
    gthread_sched_join(tasks[1], NULL);

  printf("Test finished\n");
  return 0;
}

int main() {
  init();

  return 0;
}
