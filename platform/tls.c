/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//platform/tls.c
 * info: text-includes platform-specific tls stuff (for make easy gnu)
 */

#if defined(__linux__)
#include "platform/tls_linux.c"
#else
#error "tls not supported :("
#endif
