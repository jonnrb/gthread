#pragma once

#include <atomic>

#include "absl/types/optional.h"
#include "concur/internal/waiter.h"

namespace gthread {
namespace internal {
template <typename T>
class channel_window {
 public:
  channel_window()
      : _waiter(), _reader(nullptr), _closed(false) {}

  constexpr bool is_closed() const { return _closed; }

  void close();

  template <typename U>
  absl::optional<T> write(U&& t);

  absl::optional<T> read();

 private:
  waiter _waiter;
  absl::optional<T>* _reader;
  std::atomic<bool> _closed;
};
}  // namespace internal
}  // namespace gthread

#include "concur/internal/channel_window_impl.h"
