/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//platform/tls_inline_linux.h
 * info: thread local storage inlines for Linux
 */

static inline void* gthread_tls_current_thread() {
  register void* thread __asm__("rax");
  __asm__("mov %%fs:0, %0" : "=r"(thread));
  return thread;
}
