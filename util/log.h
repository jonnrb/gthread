#pragma once

#include <string>

static inline void gthread_log(std::string message);

static inline void gthread_log_fatal(std::string message_of_death);

#include "util/log_inline.h"
