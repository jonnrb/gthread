/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//platform/memory_inline.h
 * info: inlines for platform specific memory allocation
 */

#ifndef PLATFORM_MEMORY_INLINE_H_
#define PLATFORM_MEMORY_INLINE_H_

#include <stdlib.h>

#include "util/compiler.h"

namespace gthread {
// wrapper for malloc
static inline void *allocate(size_t bytes) { return malloc(bytes); }

extern int posix_memalign(void **p, size_t a, size_t s);

// wrapper for posix_memalign
static inline void *allocate_aligned(size_t alignment, size_t bytes) {
  void *p;
  if (branch_unexpected(posix_memalign(&p, alignment, bytes))) {
    return NULL;
  } else {
    return p;
  }
}

// wrapper for free
static inline void free(void *data) { free(data); }
}  // namespace gthread

#endif  // PLATFORM_MEMORY_INLINE_H_
