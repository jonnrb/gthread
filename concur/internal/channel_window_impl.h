#pragma once

#include "concur/internal/channel_window.h"

#include <cassert>

#include "absl/types/optional.h"
#include "gthread.h"

namespace gthread {
namespace internal {
template <typename T>
void channel_window<T>::close() {
  _closed.store(true);
  _reader = nullptr;
  while (_waiter && !_waiter.unpark()) gthread::self::yield();
}

template <typename T>
absl::optional<T> channel_window<T>::read() {
  absl::optional<T> ret;

  assert(_reader == nullptr);
  _reader = &ret;

  while (true) {
    if (_closed.load()) {
      break;
    } else if (_waiter.park_if([this]() { return !_closed.load(); })) {
      assert(_reader == nullptr);
      while (_waiter && !_waiter.unpark()) gthread::self::yield();
      break;
    } else if (_waiter.swap()) {
      break;
    }
    gthread::self::yield();
  }

  return ret;
}

template <typename T>
template <typename U>
absl::optional<T> channel_window<T>::write(U&& t) {
  while (true) {
    if (_closed.load()) {
      return std::move(t);
    } else if (_waiter.park_if([this]() { return !_closed.load(); })) {
      // the channel may be closed while the writer is in the waiting state
      if (_closed.load()) {
        _reader = nullptr;
        return std::move(t);
      }

      // write into `_reader`
      assert(_reader != nullptr);
      _reader->emplace(std::move(t));
      _reader = nullptr;
      while (!_waiter.swap()) gthread::self::yield();
      break;
    } else if (_waiter && _reader != nullptr) {
      _reader->emplace(std::move(t));
      _reader = nullptr;
      while (_waiter && !_waiter.unpark()) gthread::self::yield();
      break;
    }
    gthread::self::yield();
  }

  return absl::nullopt;
}
}  // namespace internal
}  // namespace gthread
