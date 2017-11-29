#pragma once

#include "concur/internal/ring_buffer.h"

namespace gthread {
namespace internal {
template <typename T, size_t Size>
void ring_buffer<T, Size>::close() {
  _closed.store(true);
  while (_stalled && !_stalled.unpark()) gthread::self::yield();
}

template <typename T, size_t Size>
absl::optional<T> ring_buffer<T, Size>::read() {
  absl::optional<T> ret;

  auto loaded_r = _r.load();
  auto loaded_w = _w.load();

  while (loaded_w - loaded_r == 0) {
    bool loaded_closed = false;
    _stalled.park_if(
        [this, &loaded_closed]() { return !(loaded_closed = _closed.load()); });
    if (loaded_closed) return ret;
    loaded_w = _w.load();
  }

  ret.emplace(std::move(*reinterpret_cast<T*>(_buffer + (loaded_r & _mask))));
  reinterpret_cast<T*>(_buffer + (loaded_r & _mask))->~T();

  _r.store(loaded_r + 1);
  while (_stalled && !_stalled.unpark()) gthread::self::yield();
  return ret;
}

template <typename T, size_t Size>
template <typename U>
absl::optional<T> ring_buffer<T, Size>::write(U&& u) {
  if (_closed.load()) {
    return std::forward<U>(u);
  }

  auto loaded_w = _w.load();
  auto loaded_r = _r.load();

  while (loaded_w - loaded_r == Size) {
    bool loaded_closed = false;
    _stalled.park_if(
        [this, &loaded_closed]() { return !(loaded_closed = _closed.load()); });
    if (loaded_closed) return std::forward<U>(u);
    loaded_r = _r.load();
  }

  new (_buffer + (loaded_w & _mask)) T(std::forward<U>(u));

  _w.store(loaded_w + 1);
  while (_stalled && !_stalled.unpark()) gthread::self::yield();
  return absl::nullopt;
}
}  // namespace internal
}  // namespace gthread
