/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//util/log_inline.h
 * info: logging utilities
 */

#ifndef UTIL_LOG_INLINE_H_
#define UTIL_LOG_INLINE_H_

#include <stdio.h>

static inline void gthread_log_fatal(const char* message_of_death) {
  fprintf(stderr, "%s\n", message_of_death);
  fflush(stderr);
  abort();
}

#endif  // UTIL_LOG_INLINE_H_
