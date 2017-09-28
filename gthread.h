/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//gthread.h
 * info: main exported API (my_pthread.h trivially wraps this)
 */

#ifndef GTHREAD_H_
#define GTHREAD_H_

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef void* gthread_t;

typedef void* gthread_entry_t(void*);

typedef struct _gthread_attr {
  struct stack_t {
    size_t guardsize;
    size_t size;
    void* addr;
  } stack;
} gthread_attr_t;

gthread_t gthread_create(gthread_t* gthread, gthread_attr_t* attr,
                         gthread_entry_t* entry, void* arg);

bool gthread_detach(gthread_t* t);

bool gthread_join(gthread_t* t);

#endif  // GTHREAD_H_
