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



void* task1(void* arg) {
	  const char* msg = (const char*)arg;
	  printf("TASK1\n");
  	  void* p = mymalloc(10000, (gthread_task_t*)gthread_tls_current_thread());
	  void* x = mymalloc(20000, (gthread_task_t*)gthread_tls_current_thread());
	  void* z = mymalloc(10000, (gthread_task_t*)gthread_tls_current_thread());
	  myfree(x, (gthread_task_t*)gthread_tls_current_thread());
	  printInternalMemory((gthread_task_t*)gthread_tls_current_thread());
	  void* sp = shalloc(1000);
	  myfree(sp, (gthread_task_t*)gthread_tls_current_thread());
	  printShallocRegion();
	  return NULL;
}

void* task2(void* arg) {
	  const char* msg = (const char*)arg;
	  printf("TASK2\n");
  	  void* p = mymalloc(8000000, (gthread_task_t*)gthread_tls_current_thread());
  	  void* x = mymalloc(8000000, (gthread_task_t*)gthread_tls_current_thread()); //should fail
  	  //printThread((gthread_task_t*)gthread_tls_current_thread());
	  printInternalMemory((gthread_task_t*)gthread_tls_current_thread());
	  shalloc(2000);
	  printShallocRegion();
	  return NULL;
}

void* task3(void* arg){
	  const char* msg = (const char*)arg;
	  printf("TASK3\n");
	  void* p = mymalloc(8000000, (gthread_task_t*)gthread_tls_current_thread());
	  printInternalMemory((gthread_task_t*)gthread_tls_current_thread());
	  shalloc(3000);
	  printShallocRegion();
	  return NULL;

}





int init() {
  gthread_sched_handle_t tasks[26];
  char msgs[26] = {'A'};
  assert(!gthread_sched_spawn(&tasks[0], NULL, task1,
                               (void*)(&msgs[0])));
  printf("=======================================================\n");
  assert(!gthread_sched_spawn(&tasks[1], NULL, task2,
                                (void*)(&msgs[0])));
  printf("=======================================================\n");
  assert(!gthread_sched_spawn(&tasks[2], NULL, task3,
                                (void*)(&msgs[0])));

  gthread_sched_join(tasks[0], NULL);
  gthread_sched_join(tasks[1], NULL);
  gthread_sched_join(tasks[2], NULL);

  printf("Test finished\n");
  return 0;
}

int main() {
  init();

  return 0;
}
