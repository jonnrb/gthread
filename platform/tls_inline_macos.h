#pragma once

// slots 6 and 11 are reserved for WINE (so we're gonna steal em)
// DUPLICATED IN ./tls_macos.c
#define k_thread_slot_a 6
#define k_thread_slot_b 11

// nicey clang attribute allows using gs segment relative pointer vars
#define gs_relative __attribute__((address_space(256)))

static inline void *gthread_tls_current_thread() {
  return ((void *gs_relative *)0)[k_thread_slot_a];
}

#undef k_thread_slot_a
#undef k_thread_slot_b
#undef gs_relative
