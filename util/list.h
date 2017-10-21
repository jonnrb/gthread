/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//util/list.h
 * info: circular linked list that functions as a queue (not thread-safe)
 */

#ifndef UTIL_LIST_H_
#define UTIL_LIST_H_

typedef struct gthread_list_node gthread_list_node_t;

typedef gthread_list_node_t* gthread_list_t;

struct gthread_list_node {
  gthread_list_node_t* prev;
  gthread_list_node_t* next;
};

void gthread_list_push(gthread_list_t* list, gthread_list_node_t* node);

gthread_list_node_t* gthread_list_pop(gthread_list_t* list);

#endif  // UTIL_LIST_H_
