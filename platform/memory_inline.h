#ifndef PLATFORM_MEMORY_INLINE_H_
#define PLATFORM_MEMORY_INLINE_H_

#include <stdlib.h>

// wrapper for malloc
static inline void *gthread_allocate(size_t bytes) { return malloc(bytes); }

// wrapper for free
static inline void gthread_free(void *data) { free(data); }

#endif  // PLATFORM_MEMORY_INLINE_H_
