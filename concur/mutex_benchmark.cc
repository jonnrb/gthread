#include "concur/mutex.h"

#include <thread>

#include "benchmark/benchmark.h"

template <typename Mutex, typename Thread, typename Yield>
void do_mutex_comparison(benchmark::State& state, Yield& yield) {
  std::atomic<bool> done{false};
  Mutex mu{};
  bool mail_flag = false;
  int mail = 0;

  Thread writer([&mu, &mail_flag, &mail, &yield, &done]() {
    for (auto i = 0; !done.load(); ++i) {
      while (true && !done.load()) {
        std::unique_lock<Mutex> l{mu};
        if (!mail_flag) {
          mail = i;
          mail_flag = true;
          break;
        } else {
          l.unlock();
          yield();
        }
      }
    }
  });

  auto i = 0;
  for (auto _ : state) {
    while (true) {
      std::unique_lock<Mutex> l{mu};
      if (mail_flag) {
        if (mail != i) {
          std::cerr << "bad result " << mail << " != " << i << std::endl;
          abort();
        }
        mail_flag = false;
        break;
      } else {
        l.unlock();
        yield();
      }
    }
    ++i;
  }

  done.store(true);
  writer.join();
}

static void bench_gthread_mutex(benchmark::State& state) {
  do_mutex_comparison<gthread::mutex, gthread::g>(state, gthread::self::yield);
}

BENCHMARK(bench_gthread_mutex)->UseRealTime();

static void bench_stdlib_mutex(benchmark::State& state) {
  // TODO: remove when task_host works
  auto& s = gthread::sched::get();
  s.disable_timer_preemption();
  do_mutex_comparison<std::mutex, std::thread>(state, std::this_thread::yield);
  s.enable_timer_preemption();
}

BENCHMARK(bench_stdlib_mutex)->UseRealTime();

BENCHMARK_MAIN();
