/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//platform/tls.h
 * info: implement thread local storage (ELF spec)
 */

#ifndef PLATFORM_TLS_H_
#define PLATFORM_TLS_H_

#include <stdbool.h>
#include <stdlib.h>

typedef void* gthread_tls_t;

gthread_tls_t gthread_tls_allocate(size_t* tls_image_reserve,
                                   size_t* tls_image_alignment);

void gthread_tls_free(gthread_tls_t tls);

// assumes |tls| has a thread pointer with enough space before it for the tls
// base image
int gthread_tls_initialize_image(gthread_tls_t tls);

void gthread_tls_set_thread(gthread_tls_t tls, void* thread);

void* gthread_tls_get_thread(gthread_tls_t tls);

void* gthread_tls_current_thread();

gthread_tls_t gthread_tls_current();

// change the current context's tls to use |tls|
void gthread_tls_use(gthread_tls_t tls);

#endif  // PLATFORM_TLS_H_
