/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//util/list_test.c
 * info: test circular linked list as a queue
 */

#include "util/list.h"

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "util/compiler.h"

#define k_num_els 10

typedef struct {
  gthread_list_node_t n;
  uint64_t i;
} el_t;

int main() {
  el_t els[k_num_els];
  for (uint64_t i = 0; i < k_num_els; ++i) {
    els[i].i = i;
  }

  gthread_list_t list = NULL;

  for (uint64_t i = 0; i < k_num_els; ++i) {
    gthread_list_push(&list, &els[i].n);
  }

  for (uint64_t i = 0; i < k_num_els; ++i) {
    el_t* el = container_of(gthread_list_pop(&list), el_t, n);
    printf("%" PRIu64 "\n", el->i);
  }

  assert(list == NULL);

  return 0;
}
