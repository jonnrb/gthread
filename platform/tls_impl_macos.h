#pragma once

namespace gthread {
void* tls::current_thread() {
  // the magic stuff here is k_thread_slot_a which is in tls_macos.c and the
  // attribute which is macroed in the same file as gs_relative
  return ((void* __attribute__((address_space(256)))*)0)[6];
}
}  // namespace gthread
