/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//platform/memory.h
 * info: implements platform specific functionality
 */

#ifndef PLATFORM_PLATFORM_H_
#define PLATFORM_PLATFORM_H_

#include <stdlib.h>

#include "gthread.h"

// minimum stack size (probably page size). also min for guardsize.
extern const size_t GTHREAD_STACK_MIN;

/**
 * if |attrs| is null, defaults are used. if not null, it must specify
 * `attrs->stack.size` and a valid `attrs->stack.guardsize` (0 to disable).
 * the returned value in |stack| is the first address *above* the stack, so it
 * isn't valid for - let's say - passing into `gthread_free_stack()`. this is
 * so you can just start pushing to it from a new thread, as is convention.
 */
int gthread_allocate_stack(gthread_attr_t *attrs, void **stack,
                           size_t *total_stack_size);

/**
 * |stack_addr| can be any pointer within the stack. |total_stack_size| is the
 * full stack size i.e. the stack size + the guard size.
 */
int gthread_free_stack(void *stack_addr, size_t total_stack_size);

/**
 * |stack_addr| can be any pointer within the stack. |total_stack_size| is the
 * full stack size i.e. the stack size + the guard size. returns the base
 * pointer of the stack. if guardsize was specified when the stack was created,
 * this region is probably protected and accessing will make you segfault.
 */
void *gthread_get_stack_base(void *stack_addr, size_t total_stack_size);

#endif  // PLATFORM_PLATFORM_H_
