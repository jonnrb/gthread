#ifndef MY_MALLOC_PROTECTION_H_
#define MY_MALLOC_PROTECTION_H_

void gthread_malloc_protection_init();

int gthread_malloc_protection_switch_hook();

#endif  // MY_MALLOC_PROTECTION_H_
