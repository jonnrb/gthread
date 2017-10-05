/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//platform/tls.c
 * info: text-includes platform-specific tls stuff (for make easy gnu)
 */
#define _GNU_SOURCE

#include "platform/tls.h"

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "platform/memory.h"

#define baseval 15
__thread int x = baseval;

void x_inc(gthread_tls_t tls, int asserted_value) {
  if (tls != NULL) gthread_tls_use(tls);
  printf("&x: %p\n", &x);
  ++x;
  printf("x: %d\n", x - baseval);
  assert(x - baseval == asserted_value);
}

__thread struct {
  int nums[10];
  const char* str;
} y = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, "initial val"};

void y_inc(gthread_tls_t tls, int start_val, const char* str) {
  if (tls != NULL) gthread_tls_use(tls);
  for (int i = 0; i < 10; ++i) {
    printf("%d ", y.nums[i]);
    assert(y.nums[i] == (i + start_val) % 10);
    y.nums[i] = (y.nums[i] + 1) % 10;
  }
  if (start_val == 0) y.str = str;
  printf("%s\n", y.str);
  assert(strcmp(str, y.str) == 0);
}

int main() {
  printf("%p\n", gthread_tls_current_thread());

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

  if (write(fd, "def", 4) < 0) {
    perror("");
    return -1;
  }

  printf("testing XXXXXXXXXXXXXXX\n");
  printf("original\n");
  x_inc(old, 1);

  if (close(fd) < 0) {
    perror("");
    return -1;
  }

  printf("mine :)\n");
  x_inc(tls, 1);
  x_inc(NULL, 2);
  x_inc(NULL, 3);
  x_inc(NULL, 4);

  fd = open("./abc", O_RDONLY);
  if (fd < 0) {
    perror("");
    return -1;
  }

  printf("original\n");
  x_inc(old, 2);
  x_inc(NULL, 3);
  x_inc(NULL, 4);

  char buf[7];
  if (read(fd, buf, 7) < 0) {
    perror("");
    return -1;
  }
  printf("some file stuff that was also happening btw: %s\n", buf);

  printf("testing YYYYYYYYYYYYYYY\n");
  printf("original\n");
  y_inc(NULL, 0, "og thread");

  if (close(fd) < 0) {
    perror("");
    return -1;
  }

  printf("mine :)\n");
  y_inc(tls, 0, "some other thread");
  y_inc(NULL, 1, "some other thread");
  y_inc(NULL, 2, "some other thread");
  y_inc(NULL, 3, "some other thread");

  if (unlink("./abc") < 0) {
    perror("");
    return -1;
  }

  printf("original\n");
  y_inc(old, 1, "og thread");
  y_inc(NULL, 2, "og thread");
  y_inc(NULL, 3, "og thread");
  y_inc(NULL, 4, "og thread");

  gthread_tls_free(tls);

  return 0;
}
