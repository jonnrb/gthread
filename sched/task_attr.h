#pragma once

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
    .stack = {nullptr, 4 * 1024 * 1024, static_cast<size_t>(-1)},
    .alloc_tls = true  // each task gets its own thread_locals
};

/**
 * lean and mean task attributes to get in your daily dose of fiber
 */
constexpr attr k_light_attr = {
    .stack = {nullptr, 4 * 1024 * 1024, static_cast<size_t>(-1)},
    .alloc_tls = false  // no TLS so no errno and shared locale things
};
}  // namespace gthread
