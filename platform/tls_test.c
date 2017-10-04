/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//platform/tls.c
 * info: text-includes platform-specific tls stuff (for make easy gnu)
 */
#define _POSIX_C_SOURCE 200112L

#include "platform/tls.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  printf("%s\n", y.str);
  if (start_val == 0) y.str = str;
  assert(strcmp(str, y.str) == 0);
}

int main() {
  gthread_tls_t old = gthread_tls_current();
  assert(old != NULL);

  size_t total_tls_data;
  size_t tls_alignment;
  gthread_tls_t tls = gthread_tls_allocate(&total_tls_data, &tls_alignment);
  assert(tls != NULL);
  printf("total_tls_data: %zu\n", total_tls_data);
  printf("tls_alignment: %zu\n", tls_alignment);

  void* tcb = (void*)0xDEADBEEFL;
  if (total_tls_data > 0) {
    void* tcb_base;
    assert(!posix_memalign(&tcb_base, tls_alignment, total_tls_data));
    memset(tcb_base, '\0', total_tls_data);
    printf("tcb_base: %p\n", tcb_base);
    tcb = (void*)((char*)tcb_base + total_tls_data);
    gthread_tls_set_thread(tls, tcb);

    assert(!gthread_tls_initialize_image(tls));
  } else {
    gthread_tls_set_thread(tls, tcb);
  }

  assert(gthread_tls_get_thread(tls) == tcb);
  assert(gthread_tls_current_thread() != tcb);
  gthread_tls_use(tls);
  assert(gthread_tls_current_thread() == tcb);

  printf("testing XXXXXXXXXXXXXXX\n");
  printf("original\n");
  x_inc(old, 1);

  printf("mine :)\n");
  x_inc(tls, 1);
  x_inc(NULL, 2);
  x_inc(NULL, 3);
  x_inc(NULL, 4);

  printf("original\n");
  x_inc(old, 2);
  x_inc(NULL, 3);
  x_inc(NULL, 4);

  printf("testing YYYYYYYYYYYYYYY\n");
  printf("original\n");
  y_inc(NULL, 0, "og thread");

  printf("mine :)\n");
  y_inc(tls, 0, "some other thread");
  y_inc(NULL, 1, "some other thread");
  y_inc(NULL, 2, "some other thread");
  y_inc(NULL, 3, "some other thread");

  printf("original\n");
  y_inc(old, 1, "og thread");
  y_inc(NULL, 2, "og thread");
  y_inc(NULL, 3, "og thread");
  y_inc(NULL, 4, "og thread");

  gthread_tls_free(tls);

  return 0;
}
