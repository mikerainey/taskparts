#pragma once

#include <stdio.h>

#include "../include/taskparts/timing.hpp"

namespace taskparts {
  
template <typename Benchmark>
auto run_benchmark(const Benchmark& benchmark, size_t repeat=1, size_t warmup_secs=3) {
  auto seconds_of_cycles = [] (uint64_t cs) {
    return cycles::seconds_of(get_cpu_frequency_khz(), cs);
  };
  auto timed_run = [&] {
    auto st = cycles::now();
    benchmark();
    return seconds_of_cycles(cycles::since(st));
  };
  if (warmup_secs >= 1) {
    printf("======== WARMUP ========\n");
    auto warmup_start = cycles::now();
    while (seconds_of_cycles(cycles::since(warmup_start)).seconds < warmup_secs) {
      auto el = timed_run();
      printf("warmup_run %lu.%lu\n", el.seconds, el.milliseconds);
    }
    printf ("======== END WARMUP ========\n");
  }
  for (size_t i = 0; i < repeat; i++) {
    auto el = timed_run();
    printf("exectime %lu.%lu\n", el.seconds, el.milliseconds);
  }
}

} // end namespace
