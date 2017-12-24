#pragma once

/**
 * logs the |message...| to stderr
 */
template <typename... Args>
void gthread_log(Args&&... message);

/**
 * logs the |message...| to stderr and throws a runtime exception with the
 * intent of stopping the program
 */
template <typename... Args>
void gthread_log_fatal(Args&&... message);

/**
 * if compiled with NDEBUG (no asserts), this just logs the message, otherwise
 * has the behavior of `gthread_log_fatal`
 */
template <typename... Args>
void gthread_log_dfatal(Args&&... message);

#include "util/log_impl.h"
