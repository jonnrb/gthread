#ifndef MY_MALLOC_PROTECTION_H_
#define MY_MALLOC_PROTECTION_H_

#include <stdlib.h>

void gthread_malloc_protection_init();

int gthread_malloc_protection_switch_hook();

void gthread_malloc_protect_region(void* region, size_t size);

void gthread_malloc_unprotect_region(void* region, size_t size);

void gthread_malloc_protect_own_pages();

#endif  // MY_MALLOC_PROTECTION_H_
