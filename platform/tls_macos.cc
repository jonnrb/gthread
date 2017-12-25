#include "platform/tls.h"

#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "platform/memory.h"
#include "util/compiler.h"

/**
 * XNU says "ya know who really cares what everyone else does" and uses the
 * %gs segment for tls data on x86-64. k.
 *
 * however, what is nifty is that if a slot is NULL but is a valid TLS module,
 * it will be dynamically allocated.
 */

namespace gthread {
namespace {
// https://github.com/apple/darwin-libpthread/blob/master/src/internal.h
constexpr int k_num_slots = 768;
constexpr int k_num_copied_slots = 256;

// slots 6 and 11 are reserved for WINE (so we're gonna steal em)
constexpr int k_thread_slot_a = 6;
// slot b is currently unused
// constexpr int k_thread_slot_b = 11;

// nicey clang attribute allows using gs segment relative pointer vars
#define gs_relative __attribute__((address_space(256)))

typedef void* gs_relative* tls_slot_t;

tls_slot_t g_pthread_self_slot = (tls_slot_t)0;

/**
 * so xnu doesn't document its syscalls so good
 *
 * actual syscall: https://gist.github.com/aras-p/5389747
 * thanks @aras-p you da guy
 *
 * this function is used by libpthread though so it can be imported
 */
extern "C" {
extern void _thread_set_tsd_base(void*);
};

// this is terrible on several levels, but mono does it so it's okay
size_t get_pthread_slots_offset() {
  static size_t pthread_t_size = 0;
  if (pthread_t_size != 0) return pthread_t_size;

  void* pthread_self = *g_pthread_self_slot;
  uint64_t* search = (uint64_t*)pthread_self;
  while (*(void**)search != pthread_self) ++search;

  return pthread_t_size = (char*)search - (char*)pthread_self;
}
}  // namespace

tls::tls() {
  size_t pthread_t_size = get_pthread_slots_offset();
  memcpy(this, *g_pthread_self_slot, pthread_t_size);
  memset((char*)this + pthread_t_size, 0, k_num_slots * sizeof(void*));

  // set the user tcb to be this tls block
  void** tls_slots = (void**)((char*)this + pthread_t_size);
  tls_slots[0] = this;
}

tls::~tls() {
  void** tls_slots = (void**)((char*)this + get_pthread_slots_offset());

  // TODO(jonnrb): do we have to free the low slots?
  for (int i = k_num_copied_slots; i < k_num_slots; ++i) {
    if (tls_slots[i] != NULL) gthread::free(tls_slots[i]);
  }
}

size_t tls::prefix_bytes() { return 0; }

size_t tls::postfix_bytes() {
  return k_num_slots * sizeof(void*) + get_pthread_slots_offset();
}

void tls::reset() {
  void** tls_slots = (void**)((char*)this + get_pthread_slots_offset());

  // TODO(jonnrb): do we have to free the low slots?
  for (int i = k_num_copied_slots; i < k_num_slots; ++i) {
    if (tls_slots[i] != NULL) {
      free(tls_slots[i]);
      tls_slots[i] = NULL;
    }
  }
}

tls* tls::current() {
  return (tls*)((char*)*g_pthread_self_slot - sizeof(tls));
}

void tls::set_thread(void* thread) {
  void** tls_slots = (void**)((char*)this + get_pthread_slots_offset());
  tls_slots[k_thread_slot_a] = thread;
}

void* tls::get_thread() {
  void** tls_slots = (void**)((char*)this + get_pthread_slots_offset());
  return tls_slots[k_thread_slot_a];
}

void tls::use() {
  void** tls_slots = (void**)((char*)this + get_pthread_slots_offset());
  _thread_set_tsd_base((void*)tls_slots);
}

}  // namespace gthread
