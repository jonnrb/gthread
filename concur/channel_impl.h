#pragma once

#include "concur/channel.h"

namespace gthread {
template <typename T>
std::optional<T> channel_reader<T>::read() {
  if (!_window) {
    return std::nullopt;
  }
  return _window->read();
}

template <typename T>
void channel_reader<T>::reset() {
  if (_window) {
    _window->close();
    _window.reset();
  }
}

template <typename T>
template <typename U>
std::optional<T> channel_writer<T>::write(U&& val) {
  if (!_window) {
    return std::optional<T>{std::forward<U>(val)};
  }
  return _window->write(std::forward<U>(val));
}

template <typename T>
void channel_writer<T>::reset() {
  if (_window) {
    _window->close();
    _window.reset();
  }
}

template <typename T>
channel<T> make_channel() {
  auto window = std::make_shared<internal::channel_window<T>>();
  return {channel_reader<T>{window}, channel_writer<T>{window}};
}
}  // namespace gthread
