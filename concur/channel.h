#pragma once

#include <cstdlib>

namespace gthread {
template <typename Read, typename Write>
struct channel {
  Read read;
  Write write;
};

/**
 * makes a synchronous channel
 */
template <typename T>
auto make_channel();

/*
 * makes a channel with an internal ring buffer
 *
 * |size| must be a power of 2
 */
template <typename T, size_t Size>
auto make_buffered_channel();
};  // namespace gthread

#include "concur/channel_impl.h"
