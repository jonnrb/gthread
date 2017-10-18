#include "mutex.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

gthread_mutex_t mutex;
int mutextarget;

void printMutex(char* arg) {
  printf("|||||||||||||||||||||||||||||||||||||||||||||\n");
  printf("-----MUTEX STRUCTURE STATE-----\n");
  printf("%s\n", arg);
  if (mutex.state == LOCKED) {
    // printf("Mutex is LOCKED by task: %c \n", mutex.which_task);  //delete
    // comment for test
  } else {
    printf("Mutex is UNLOCKED\n");
  }
  printf("|||||||||||||||||||||||||||||||||||||||||||||\n");

  return;
}

void* important_task(void* arg) {
  const char* msg = (const char*)arg;
  clock_t start = clock();
  for (clock_t c = clock();; c = clock()) {
    if ((c - start) > CLOCKS_PER_SEC) {
      printf("Starting - I am task: \"%c\"\n", *msg);
      start = c;
      printf("Mutex Variable value BEFORE calculation: %d\n", mutextarget);
      printf("-----------------------------------------------\n");
      printMutex("BEFORE LOCK");
      gthread_mutex_lock(&mutex);
      // mutex.which_task = *msg; //delete comment for test
      printMutex("AFTER LOCK/BEFORE UNLOCK");
      mutextarget = mutextarget + 1;
      printf("Mutex Variable value AFTER calculation: %d\n", mutextarget);
      gthread_sched_yield();
      printf("About to unlock - I am task: \"%c\"\n", *msg);
      gthread_mutex_unlock(&mutex);
      printMutex("AFTER UNLOCK");
    } else {
      gthread_sched_yield();
    }
  }
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
  printf("done and stuff\n");
  while (true) gthread_sched_yield();
  return 0;
}

int main() {
  mutextarget = 0;
  printf("Mutex INIT result first time: %d\n",
         gthread_mutex_init(&mutex, NULL));
  gthread_sched_init();
  printf("Scheduler initialized.\n");
  init();
  printf("FIRST Gthread destroyed return integer: %d\n",
         gthread_mutex_destroy(&mutex));
  printf("SECOND Gthread destroyed return integer: %d\n",
         gthread_mutex_destroy(&mutex));
  return 1;
}
