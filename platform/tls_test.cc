/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//platform/tls.cc
 * info: text-includes platform-specific tls stuff (for make easy gnu)
 */

#include "platform/tls.h"

#include <cassert>
#include <chrono>
#include <iostream>

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "platform/clock.h"
#include "platform/memory.h"

constexpr int baseval = 15;
thread_local int x = baseval;

void x_inc(gthread_tls_t tls, int asserted_value) {
  if (tls != NULL) gthread_tls_use(tls);
  std::cout << "&x: " << &x << std::endl;
  ++x;
  std::cout << "x: " << (x - baseval) << std::endl;
  assert(x - baseval == asserted_value);
}

thread_local struct {
  int nums[10];
  const char* str;
} y = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, "initial val"};

void y_inc(gthread_tls_t tls, int start_val, const char* str) {
  if (tls != NULL) gthread_tls_use(tls);
  for (int i = 0; i < 10; ++i) {
    std::cout << y.nums[i] << " ";
    assert(y.nums[i] == (i + start_val) % 10);
    y.nums[i] = (y.nums[i] + 1) % 10;
  }
  if (start_val == 0) y.str = str;
  std::cout << y.str << std::endl;
  assert(strcmp(str, y.str) == 0);
}

int main() {
  std::cout << gthread_tls_current_thread() << std::endl;
  gthread_tls_set_thread(gthread_tls_current(), (void*)0xbacabacaL);

  int fd = open("./abc", O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
  if (fd < 0) {
    perror("");
    return -1;
  }

  gthread_tls_t old = gthread_tls_current();
  assert(old != NULL);

  if (write(fd, "abc", 3) < 0) {
    perror("");
    return -1;
  }

  gthread_tls_t tls = gthread_tls_allocate();
  assert(tls != NULL);

  void* tcb = (void*)0xDEADBEEFL;
  gthread_tls_set_thread(tls, tcb);

  assert(gthread_tls_get_thread(tls) == tcb);
  assert(gthread_tls_current_thread() != tcb);
  gthread_tls_use(tls);
  assert(gthread_tls_current_thread() == tcb);

  gthread_tls_t other = gthread_tls_allocate();
  void* other_tcb = (void*)0xf00dea7e5L;
  gthread_tls_set_thread(other, other_tcb);
  gthread_tls_use(other);
  assert(gthread_tls_current_thread() == other_tcb);

  if (write(fd, "def", 4) < 0) {
    perror("");
    return -1;
  }

  std::cout << "testing XXXXXXXXXXXXXXX" << std::endl;
  std::cout << "original" << std::endl;
  x_inc(old, 1);

  if (close(fd) < 0) {
    perror("");
    return -1;
  }

  std::cout << "mine :)" << std::endl;
  x_inc(tls, 1);
  x_inc(NULL, 2);
  x_inc(NULL, 3);
  x_inc(NULL, 4);

  fd = open("./abc", O_RDONLY);
  if (fd < 0) {
    perror("");
    return -1;
  }

  std::cout << "original" << std::endl;
  x_inc(old, 2);
  x_inc(NULL, 3);
  x_inc(NULL, 4);

  char buf[7];
  if (read(fd, buf, 7) < 0) {
    perror("");
    return -1;
  }
  std::cout << "some file stuff that was also happening btw: " << buf
            << std::endl;

  std::cout << "testing YYYYYYYYYYYYYYY" << std::endl;
  std::cout << "original" << std::endl;
  y_inc(NULL, 0, "og thread");

  if (close(fd) < 0) {
    perror("");
    return -1;
  }

  std::cout << "mine :)" << std::endl;
  y_inc(tls, 0, "some other thread");
  y_inc(NULL, 1, "some other thread");
  y_inc(NULL, 2, "some other thread");
  y_inc(NULL, 3, "some other thread");

  if (unlink("./abc") < 0) {
    perror("");
    return -1;
  }

  std::cout << "original" << std::endl;
  y_inc(old, 1, "og thread");
  y_inc(NULL, 2, "og thread");
  y_inc(NULL, 3, "og thread");
  y_inc(NULL, 4, "og thread");

  std::cout << "poisoning main tls" << std::endl;
  x = 172;
  y.nums[0] = -9;
  y.nums[5] = -3;

  std::cout << "resetting mine" << std::endl;
  assert(!gthread_tls_reset(tls));

  x_inc(tls, 1);
  x_inc(NULL, 2);
  x_inc(NULL, 3);
  x_inc(NULL, 4);

  y_inc(NULL, 0, "some other thread");
  y_inc(NULL, 1, "some other thread");
  y_inc(NULL, 2, "some other thread");
  y_inc(NULL, 3, "some other thread");
  gthread_tls_use(old);

  constexpr uint64_t k_num_tls_switches = 1000 * 1000;
  auto start = gthread::thread_clock::now();
  for (uint64_t i = 0; i < k_num_tls_switches; ++i) {
    gthread_tls_use(tls);
    __asm__ __volatile__("" ::: "memory");
    gthread_tls_use(old);
    __asm__ __volatile__("" ::: "memory");
  }
  auto end = gthread::thread_clock::now();
  auto elapsed =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  std::cout << "  " << (k_num_tls_switches * 2) << " tls switches in "
            << elapsed.count() << " microseconds" << std::endl;

  std::cout << "  " << k_num_tls_switches * 2 / ((double)elapsed.count())
            << " tls switches / microsecond" << std::endl;

  gthread_tls_free(tls);

  return 0;
}
