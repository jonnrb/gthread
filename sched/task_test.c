/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//sched/task_test.c
 * info: test task switching and spawning (WIP)
 */

#include "sched/task.h"

gthread_task_t* tasks[1024] = {NULL};

void* important_task(void* arg) {
  uint64_t id = ;
  clock_t start = clock();
  for (clock_t c = clock();; c = clock()) {
    if ((c - start) > CLOCKS_PER_SEC) {
      printf("%p:", t);
      for (int i = 0; i < 16; ++i) printf(" %.5llu", t->slice_times[i]);
      printf("\n");
      start = c;
    } else {
      gthread_switch_to_task(tasks[0]);
    }
  }
}

int main() {
  // TODO: DO
  return 1;
}
