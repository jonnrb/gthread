/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//util/log_inline.h
 * info: logging utilities
 */

#ifndef UTIL_LOG_INLINE_H_
#define UTIL_LOG_INLINE_H_

#include <iostream>

static inline void gthread_log(std::string message) {
  std::cerr << message << std::endl;
}

static inline void gthread_log_fatal(std::string message_of_death) {
  std::cerr << message_of_death << std::endl;
  abort();
}

#endif  // UTIL_LOG_INLINE_H_
