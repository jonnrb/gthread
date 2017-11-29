#pragma once

#include "concur/channel.h"

#include <memory>

#include "concur/internal/channel_window.h"
#include "concur/internal/ring_buffer.h"
#include "util/close_wrapper.h"

namespace gthread {
namespace internal {
template <typename T, typename Monitor>
auto make_wishbone_channel() {
  auto monitor = std::make_shared<Monitor>();
  using wrapper = internal::close_wrapper<decltype(monitor)>;

  auto read = [w = wrapper(monitor)]() { return w->read(); };

  auto write = [w = wrapper(monitor)](auto&& t) {
    return w->write(std::forward<T>(t));
  };

  return channel<decltype(read), decltype(write)>{std::move(read),
                                                  std::move(write)};
}
}  // namespace internal

template <typename T>
auto make_channel() {
  return internal::make_wishbone_channel<T, internal::channel_window<T>>();
}

template <typename T, size_t Size>
auto make_buffered_channel() {
  return internal::make_wishbone_channel<T, internal::ring_buffer<T, Size>>();
}
}  // namespace gthread
