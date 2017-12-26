#pragma once

#include <cstdint>
#include <cstdlib>

namespace gthread {
/**
 * Thread Local Storage allows for global data that is relatively local to a
 * context, i.e. the values can be all switched during a context switch.
 *
 * C++ allows declaring these with the `thread_local` keyword. this doesn't
 * support thread local objects with non-trivial constructors.
 *
 * ELF TLS ABI document: https://www.akkadia.org/drepper/tls.pdf
 */
class tls {
 public:
  /**
   * this class NEEDS to be in-place allocated. there should be
   * `prefix_bytes()` before this and `postfix_bytes()` after.
   */
  tls();

  /**
   * since this class is in-place allocated, the destructor needs to be called
   * directly, which is unusual
   *
   * tls* my_tls = new(some_bytes_somewhere) tls();
   * ...
   * my_tls->~tls();
   */
  ~tls();

  /**
   * the amount of bytes that must be reserved before and after the address
   * where the tls object is constructed. the bytes after include the size of
   * this object.
   *
   * char* storage = new char[tls::prefix_bytes() + tls::postfix_bytes()];
   * tls* my_tls = new (storage + tls::prefix_bytes()) tls();
   */
  static size_t prefix_bytes();
  static size_t postfix_bytes();

  /**
   * resets the thread local data stored for |tls|
   */
  void reset();

  /**
   * sets the thread pointer (user data) on |tls|
   */
  void set_thread(void* thread);

  /**
   * returns the thread pointer that is set on |tls|
   */
  void* get_thread();

  /**
   * returns the thread pointer for the current context's tls
   */
  static inline void* current_thread();

  /**
   * returns the tls used in the current execution context
   */
  static tls* current();

  /**
   * change the current context's tls to use |tls|
   */
  void use();

 private:
  uint8_t* after() { return reinterpret_cast<uint8_t*>(this) + sizeof(tls); }
};
}  // namespace gthread

#if defined(__APPLE__) && defined(__MACH__)
#include "platform/tls_impl_macos.h"
#elif defined(__linux__)
#include "platform/tls_impl_linux.h"
#else
#error "tls not supported :("
#endif
