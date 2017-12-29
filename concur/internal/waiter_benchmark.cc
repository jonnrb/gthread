#include "concur/internal/waiter.h"

#include <benchmark/benchmark.h>

#include "gthread.h"

static void benchmark_waiter(benchmark::State& state) {
  gthread::internal::waiter w;
  auto done_flag = false;

  gthread::g bg([&w, &done_flag]() {
    while (!w.park()) {
    }
    while (!done_flag) {
      while (!w.swap()) {
      }
    }
  });

  for (auto _ : state) {
    while (!w.swap()) {
    }
  }

  done_flag = true;
  while (!w.unpark()) {
  }

  bg.join();
}

BENCHMARK(benchmark_waiter);

BENCHMARK_MAIN()
