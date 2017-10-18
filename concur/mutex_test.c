#include "concur/mutex.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "platform/clock.h"

static gthread_mutex_t mutex;
static int mutextarget = 0;
static bool go = false;

void* important_task(void* arg) {
  const char* msg = (const char*)arg;

  while (!go) gthread_sched_yield();

  for (int i = 0; i < 26; ++i) {
    gthread_mutex_lock(&mutex);
    printf("mutex locked by task %c\n", *msg);
    printf("Mutex Variable value BEFORE calculation: %d\n", mutextarget);
    mutextarget = mutextarget + 1;
    gthread_nsleep(5 * 1000 * 1000);
    printf("Mutex Variable value AFTER calculation: %d\n", mutextarget);
    printf("mutex unlocked by task %c\n", *msg);
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
