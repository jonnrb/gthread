#include "platform/clock.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

int main() {
  for (int i = 0; i < 10; ++i) {
    int64_t t = gthread_clock_real();
    assert(t >= 0);
    t = t / (1000 * 1000) * (1000 * 1000);
    printf("realtime clock masked to ms: %" PRId64 "\n", t);
    printf("slept for %" PRIu64 " real ns\n",
           gthread_nsleep(100 * 1000 * 1000));
  }

  int64_t last = gthread_clock_process();
  assert(last >= 0);
  last = last / (1000 * 1000) * (1000 * 1000);
  printf("slept for %" PRIu64 " real ns\n", gthread_nsleep(100 * 1000 * 1000));
  for (int i = 0; i < 10; ++i) {
    int64_t t = gthread_clock_process();
    assert(t >= 0);
    t = t / (1000 * 1000) * (1000 * 1000);
    printf("process clock masked to ms: %" PRId64 "\n", t);
    printf("process clock elapsed in ms: %" PRId64 "\n", t - last);
    last = t;
    printf("slept for %" PRIu64 " real ns\n",
           gthread_nsleep(100 * 1000 * 1000));
  }

  return 0;
}
