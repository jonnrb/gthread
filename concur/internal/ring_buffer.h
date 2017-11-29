#pragma once

#include "absl/types/optional.h"
#include "concur/internal/waiter.h"
#include "gthread.h"

namespace gthread {
namespace internal {
template <typename T, size_t Size>
class ring_buffer {
  static_assert(Size != 0 && (Size & (Size - 1)) == 0,
                "Size must be a power of 2");

 public:
  ring_buffer() : _buffer(), _r(0), _w(0), _closed(false), _stalled() {}

  void close();

  absl::optional<T> read();

  template <typename U>
  absl::optional<T> write(U&& u);

 private:
  static constexpr size_t _mask = Size - 1;

  typename std::aligned_storage<sizeof(T), alignof(T)>::type _buffer[Size];

  std::atomic<size_t> _r;
  std::atomic<size_t> _w;

  std::atomic<bool> _closed;

  waiter _stalled;
};
}  // namespace internal
}  // namespace gthread

#include "concur/internal/ring_buffer_impl.h"
