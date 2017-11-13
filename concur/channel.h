#pragma once

#include <type_traits>

#include "concur/internal/channel_window.h"

namespace gthread {
template <typename T>
class channel_reader {
  static_assert(!std::is_reference<T>::value,
                "cannot make a channel_reader with a reference type");

 public:
  channel_reader() : _window(nullptr) {}
  explicit channel_reader(std::shared_ptr<internal::channel_window<T>> window)
      : _window(std::move(window)) {}
  channel_reader(const channel_reader<T>&) = delete;
  channel_reader(channel_reader<T>&&) = default;

  ~channel_reader() { reset(); }

  channel_reader<T>& operator=(channel_reader<T>&) = delete;
  channel_reader<T>& operator=(channel_reader<T>&& other) = default;

  absl::optional<T> read();

  void reset();

 private:
  std::shared_ptr<internal::channel_window<T>> _window;
};

template <typename T>
class channel_writer {
  static_assert(!std::is_reference<T>::value,
                "cannot make a channel_writer with a reference type");

  using write_t = absl::conditional_t<std::is_integral<T>::value, T, T&&>;

 public:
  channel_writer() : _window(nullptr) {}
  explicit channel_writer(std::shared_ptr<internal::channel_window<T>> window)
      : _window(std::move(window)) {}
  channel_writer(const channel_writer<T>&) = delete;
  channel_writer(channel_writer<T>&&) = default;

  ~channel_writer() { reset(); }

  channel_writer<T>& operator=(channel_writer<T>&) = delete;
  channel_writer<T>& operator=(channel_writer<T>&& other) = default;

  template <typename U>
  absl::optional<T> write(U&& val);

  void reset();

 private:
  std::shared_ptr<internal::channel_window<T>> _window;
};

template <typename Input, typename Output>
struct pipe {
  channel_reader<Output> r;
  channel_writer<Input> w;
};

template <typename T>
using channel = pipe<T, T>;

template <typename T>
channel<T> make_channel();
};  // namespace gthread

#include "concur/channel_impl.h"
