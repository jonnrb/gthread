/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//platform/tls_inline_linux.h
 * info: thread local storage inlines for Linux
 */

#ifndef PLATFORM_TLS_INLINE_LINUX_H_
#define PLATFORM_TLS_INLINE_LINUX_H_

#include "util/compiler.h"

static inline void* gthread_tls_current_thread() {
  register void* thread __asm__("rax");
  __asm__("mov %%fs:%P1, %0" : "=r"(thread) : "i"(16));
  return thread;
}

#endif  // PLATFORM_TLS_INLINE_LINUX_H_
