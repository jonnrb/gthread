#pragma once

namespace gthread {
void* tls::current_thread() {
  register void* thread __asm__("rax");
  __asm__("mov %%fs:%P1, %0" : "=r"(thread) : "i"(16));
  return thread;
}
}  // namespace gthread
