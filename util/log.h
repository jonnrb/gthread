/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//util/log.h
 * info: logging utilities
 */

#ifndef UTIL_LOG_H_
#define UTIL_LOG_H_

#include <string>

static inline void gthread_log(std::string message);

static inline void gthread_log_fatal(std::string message_of_death);

#include "util/log_inline.h"

#endif  // UTIL_LOG_H_
