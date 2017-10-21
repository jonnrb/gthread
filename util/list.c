/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//util/list.c
 * info: circular linked list that functions as a queue (not thread-safe)
 */

#include "util/list.h"

#include <errno.h>
#include <stdlib.h>

#include "util/compiler.h"

void gthread_list_push(gthread_list_t* list, gthread_list_node_t* node) {
  if (branch_unexpected(list == NULL)) {
    errno = EINVAL;
    return;
  }

  if (*list == NULL) {
    *list = node;
    node->prev = node;
    node->next = node;
  } else {
    node->prev = (*list)->prev;
    node->prev->next = node;
    node->next = *list;
    (*list)->prev = node;
  }
}

gthread_list_node_t* gthread_list_pop(gthread_list_t* list) {
  if (branch_unexpected(list == NULL)) {
    errno = EINVAL;
    return NULL;
  }

  if (*list == NULL) {
    return NULL;
  }

  gthread_list_node_t* ret = *list;

  if ((*list)->prev == *list) {
    *list = NULL;
  } else {
    (*list)->prev->next = (*list)->next;
    (*list)->next->prev = (*list)->prev;
    *list = (*list)->next;
  }

  return ret;
}
