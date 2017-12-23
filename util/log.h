#pragma once

#include <string>

template <typename... Args>
void gthread_log(Args&&... message);

template <typename... Args>
void gthread_log_fatal(Args&&... message);

#include "util/log_impl.h"
