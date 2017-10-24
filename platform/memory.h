/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//platform/memory.h
 * info: implements platform specific functionality
 */

#ifndef PLATFORM_MEMORY_H_
#define PLATFORM_MEMORY_H_

#include <stdlib.h>

#include "gthread.h"

namespace gthread {

// minimum stack size (probably page size). also min for guardsize.
extern const size_t k_stack_min;

// wrapper for malloc
static inline void* allocate(size_t bytes);

// wrapper for posix_memalign
static inline void* allocate_aligned(size_t alignment, size_t bytes);

// wrapper for free
static inline void free(void* data);

/**
 * if |attrs| is null, defaults are used. if not null, it must specify
 * `attrs->stack.size` and a valid `attrs->stack.guardsize` (0 to disable).
 * the returned value in |stack| is the first address *above* the stack, so it
 * isn't valid for - let's say - passing into `free_stack()`. this is
 * so you can just start pushing to it from a new thread, as is convention.
 */
int allocate_stack(const gthread::attr& a, void** stack,
                   size_t* total_stack_size);

/**
 * |stack_addr| can be any pointer within the stack. |total_stack_size| is the
 * full stack size i.e. the stack size + the guard size.
 */
int free_stack(void* stack_addr, size_t total_stack_size);

};  // namespace gthread

#include "platform/memory_inline.h"

#endif  // PLATFORM_MEMORY_H_
