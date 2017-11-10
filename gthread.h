#pragma once

#include <cstdint>
#include <cstdlib>

namespace gthread {

struct attr {
  struct stack_t {
    void* addr;
    size_t size;
    size_t guardsize;
  } stack;

  bool alloc_tls;
};

/**
 * task attributes that "feel" like a kernel thread: comes with a large stack
 * and thread-local storage turned on
 */
constexpr attr k_default_attr = {
    .stack =
        {
            .addr = nullptr,
            .size = 4 * 1024 * 1024,              // 4 MB stack
            .guardsize = static_cast<size_t>(-1)  // auto guard
        },
    .alloc_tls = true  // each task gets its own thread_locals
};

/**
 * lean and mean task attributes to get in your daily dose of fiber
 */
constexpr attr k_light_attr = {
    .stack =
        {
            .addr = nullptr,
            .size = 8 * 1024,                     // 8 KB stack
            .guardsize = static_cast<size_t>(-1)  // auto guard
        },
    .alloc_tls = false  // no TLS so no errno and shared locale things
};

};  // namespace gthread
