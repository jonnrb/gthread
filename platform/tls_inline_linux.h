#pragma once

#include "util/compiler.h"

static inline void* gthread_tls_current_thread() {
  register void* thread __asm__("rax");
  __asm__("mov %%fs:%P1, %0" : "=r"(thread) : "i"(16));
  return thread;
}
