#pragma once

namespace gthread {
namespace internal {
/**
 * RAII smart pointer wrapper that calls a `close()` method on the resource
 * before releasing the smart pointer
 *
 * intentionally move only to use with channels
 */
template <typename ResourcePtr>
class close_wrapper {
 public:
  explicit close_wrapper(ResourcePtr resource) : _resource(resource) {}

  close_wrapper(const close_wrapper<ResourcePtr>&) = delete;
  close_wrapper(close_wrapper<ResourcePtr>&&) = default;

  close_wrapper<ResourcePtr>& operator=(const close_wrapper<ResourcePtr>&) =
      delete;
  close_wrapper<ResourcePtr>& operator=(close_wrapper<ResourcePtr>&&) = default;

  ~close_wrapper() {
    if (_resource) _resource->close();
  }

  const ResourcePtr& operator->() const { return _resource; }

 private:
  ResourcePtr _resource;
};
}  // namespace internal
}  // namespace gthread
