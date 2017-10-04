/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//platform/tls.c
 * info: text-includes platform-specific tls stuff (for make easy gnu)
 */

#if defined(__APPLE__) && defined(__MACH__)
#include "platform/tls_macos.c"
#elif defined(__linux__)
#include "platform/tls_linux.c"
#else
#error "tls not supported :("
#endif
