/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//platform/memory_inline.h
 * info: inlines for platform specific memory allocation
 */

#ifndef PLATFORM_MEMORY_INLINE_H_
#define PLATFORM_MEMORY_INLINE_H_

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#else
#define GTHREAD_POSIX_C_SOURCE_WAS_DEF
#endif

#include <stdlib.h>

#include "util/compiler.h"

// wrapper for malloc
static inline void *gthread_allocate(size_t bytes) { return malloc(bytes); }

// wrapper for posix_memalign
static inline void *gthread_allocate_aligned(size_t alignment, size_t bytes) {
  void *p;
  if (branch_unexpected(posix_memalign(&p, alignment, bytes))) {
    return NULL;
  } else {
    return p;
  }
}

// wrapper for free
static inline void gthread_free(void *data) { free(data); }

#ifndef GTHREAD_POSIX_C_SOURCE_WAS_DEF
#undef _POSIX_C_SOURCE
#endif

#endif  // PLATFORM_MEMORY_INLINE_H_
