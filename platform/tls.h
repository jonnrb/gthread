/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//platform/tls.h
 * info: implement thread local storage (mostly ELF spec)
 */

/**
 * ELF TLS ABI document: https://www.akkadia.org/drepper/tls.pdf
 */

#ifndef PLATFORM_TLS_H_
#define PLATFORM_TLS_H_

#include <stdbool.h>
#include <stdlib.h>

typedef void* gthread_tls_t;

/**
 * allocates a tls dtv (dynamic thread vector) which is returned as an opaque
 * pointer conforming to the current platform. when the thread pointer is set
 * on the tls, it should have |tls_image_reserve| bytes allocated before that
 * pointer aligned to a |tls_image_alignment| boundary.
 */
gthread_tls_t gthread_tls_allocate(size_t* tls_image_reserve,
                                   size_t* tls_image_alignment);

/**
 * frees the tls data and the dtv.
 */
void gthread_tls_free(gthread_tls_t tls);

/**
 * assumes |tls| has a thread pointer with enough space before it for the tls
 * base image
 */
int gthread_tls_initialize_image(gthread_tls_t tls);

/**
 * sets the thread pointer on the tls vector. it should have the appropriate
 * storage available before this address.
 */
void gthread_tls_set_thread(gthread_tls_t tls, void* thread);

/**
 * returns the thread vector that is set on |tls|
 */
void* gthread_tls_get_thread(gthread_tls_t tls);

/**
 * returns the thread vector for the current context's tls
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
