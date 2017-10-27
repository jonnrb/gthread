#pragma once

namespace gthread {
namespace internal {

template <bool enabled>
sched_stats<enabled>::sched_stats(thread_clock::duration interval)
    : _used_time{0}, _interval(interval) {}

template <>
int sched_stats<false>::init() {
  return 0;
}

template <>
int sched_stats<true>::init() {
  _start_time = thread_clock::now();

  _self_handle = sched::spawn(k_default_attr, &sched_stats<true>::thread, this);
  if (!_self_handle) {
    return -1;
  }

  return 0;
}

template <bool enabled>
void sched_stats<enabled>::print() {
  using float_duration = std::chrono::duration<float>;

  std::cerr << "printing scheduler stats every "
            << std::chrono::duration_cast<float_duration>(_interval).count()
            << "s" << std::endl;

  auto now = thread_clock::now();
  auto last = now;

  while (true) {
    if (now - last < std::chrono::seconds{5}) {
      sched::yield();
      now = thread_clock::now();
      continue;
    }
    last = now;

    std::cerr
        << "process time since sched start   "
        << std::chrono::duration_cast<float_duration>(now - _start_time).count()
        << " s\nprocess time spent in scheduler  "
        << std::chrono::duration_cast<float_duration>(_used_time).count()
        << " s\nratio spent in scheduler         "
        << (std::chrono::duration_cast<float_duration>(_used_time).count() /
            std::chrono::duration_cast<float_duration>(now - _start_time)
                .count())
        << std::endl;
  }
}

}  // namespace internal
}  // namespace gthread
