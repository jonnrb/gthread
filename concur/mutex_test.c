/**
 * author: Khalid Akash, JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//concur/mutex_test.c
 * info: tests mutex by locking and unlocking in a tight loop across threads
 */

#include "concur/mutex.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "platform/clock.h"

static gthread_mutex_t mutex;
static char last_task_with_mutex = '-';
static int mutextarget = 0;
static bool go = false;

void* important_task(void* arg) {
  const char* msg = (const char*)arg;

  while (!go) gthread_sched_yield();

  for (int i = 0; i < 26; ++i) {
    gthread_mutex_lock(&mutex);
    if (last_task_with_mutex != *msg) {
      printf("mutex hot potato! %c -> %c\n", last_task_with_mutex, *msg);
    }
    last_task_with_mutex = *msg;
    mutextarget = mutextarget + 1;
    gthread_nsleep(5 * 1000 * 1000);
    gthread_mutex_unlock(&mutex);
  }

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

  go = true;

  for (int i = 0; i < 26; ++i) {
    gthread_sched_join(tasks[i], NULL);
  }
  printf("done and stuff\n");
  assert(mutextarget == 26 * 26);
  return 0;
}

int main() {
  mutextarget = 0;
  assert(!gthread_mutex_init(&mutex, NULL));
  init();
  assert(!gthread_mutex_destroy(&mutex));
  fprintf(stderr, "destroying destroyed mutex should err: ");
  assert(gthread_mutex_destroy(&mutex));
  return 0;
}
