/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//gthread.h
 * info: main exported API
 */

#ifndef GTHREAD_H_
#define GTHREAD_H_

#include <stdint.h>
#include <stdlib.h>

namespace gthread {

struct attr {
  struct stack_t {
    void* addr;
    size_t size;
    size_t guardsize;
  } stack;
};

constexpr attr k_default_attr = {
    .stack = {
        .addr = NULL,
        .size = 4 * 1024 * 1024,              // 4 MB stack
        .guardsize = static_cast<size_t>(-1)  // auto guard
    }};

};  // namespace gthread

#endif  // GTHREAD_H_
