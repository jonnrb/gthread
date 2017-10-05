/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//platform/tls.h
 * info: implement thread local storage (mostly ELF spec)
 */

/**
 * Thread Local Storage allows for global data that is relatively local to a
 * context, i.e. the values can be all switched during a context switch.
 *
 * the standard gnu extension allows declaring static variables with a
 * `__thread` prefix and C11 uses the `_Thread_local` prefix
 *
 * ELF TLS ABI document: https://www.akkadia.org/drepper/tls.pdf
 */

#ifndef PLATFORM_TLS_H_
#define PLATFORM_TLS_H_

#include <stdbool.h>
#include <stdlib.h>

typedef struct gthread_tls* gthread_tls_t;

/**
 * allocates thread local storage for a context. the returned thread local
 * storage is initialized to default values.
 */
gthread_tls_t gthread_tls_allocate();

/**
 * resets the thread local data stored for |tls|
 */
int gthread_tls_reset(gthread_tls_t tls);

/**
 * frees the tls data
 */
void gthread_tls_free(gthread_tls_t tls);

/**
 * sets the thread pointer (user data) on |tls|
 */
void gthread_tls_set_thread(gthread_tls_t tls, void* thread);

/**
 * returns the thread pointer that is set on |tls|
 */
void* gthread_tls_get_thread(gthread_tls_t tls);

/**
 * returns the thread pointer for the current context's tls
 */
static inline void* gthread_tls_current_thread();

/**
 * returns the tls used in the current execution context
 */
gthread_tls_t gthread_tls_current();

/**
 * change the current context's tls to use |tls|
 */
void gthread_tls_use(gthread_tls_t tls);

#if defined(__APPLE__) && defined(__MACH__)
#include "platform/tls_inline_macos.h"
#elif defined(__linux__)
#include "platform/tls_inline_linux.h"
#else
#error "tls not supported :("
#endif

#endif  // PLATFORM_TLS_H_
