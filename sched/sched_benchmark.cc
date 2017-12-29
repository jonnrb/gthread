#include "sched/sched.h"

#include <iostream>
#include <thread>

#include <benchmark/benchmark.h>

static void benchmark_std_thread(benchmark::State& state) {
  std::vector<std::thread> threads(state.range(0));
  std::vector<uint64_t> shared(state.range(0), 0);

  for (auto _ : state) {
    auto i = 0;
    for (auto& t : threads) {
      t = std::thread([&shared, i]() { ++shared[i]; });
      ++i;
    }

    for (auto& t : threads) {
      t.join();
    }
  }
}

BENCHMARK(benchmark_std_thread)->Range(1 << 3, 1 << 11);

void* something(void* arg) {
  auto* thing = reinterpret_cast<uint64_t*>(arg);
  ++*thing;
  return nullptr;
}

static void benchmark_sched_default_attr(benchmark::State& state) {
  std::vector<gthread::sched::handle> threads(state.range(0));
  std::vector<uint64_t> shared(state.range(0), 0);

  for (auto _ : state) {
    auto i = 0;
    for (auto& t : threads) {
      t = gthread::sched::spawn(gthread::k_default_attr, something,
                                reinterpret_cast<void*>(shared.data() + i));
    }

    for (auto& t : threads) {
      gthread::sched::join(&t, nullptr);
    }
  }
}

BENCHMARK(benchmark_sched_default_attr)->Range(1 << 3, 1 << 11);

static void benchmark_sched_light_attr(benchmark::State& state) {
  std::vector<gthread::sched::handle> threads(state.range(0));
  std::vector<uint64_t> shared(state.range(0), 0);

  for (auto _ : state) {
    auto i = 0;
    for (auto& t : threads) {
      t = gthread::sched::spawn(gthread::k_light_attr, something,
                                reinterpret_cast<void*>(shared.data() + i));
    }

    for (auto& t : threads) {
      gthread::sched::join(&t, nullptr);
    }
  }
}

BENCHMARK(benchmark_sched_light_attr)->Range(1 << 3, 1 << 11);

BENCHMARK_MAIN()
