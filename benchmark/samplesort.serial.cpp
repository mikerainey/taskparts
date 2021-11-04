#include <taskparts/benchmark.hpp>
#include "samplesort.hpp"

int main() {
  size_t repeat = taskparts::cmdline::parse_or_default_long("repeat", 0);
  if (repeat == 0) {
    parlay::benchmark_taskparts([&] (auto sched) {
      benchmark_no_swaps();
    }, [&] (auto sched) {
      gen_input();
    });
  } else {
    parlay::benchmark_taskparts([&] (auto sched) {
      benchmark_with_swaps(repeat);
    }, [&] (auto sched) {
      gen_input();
    });
  }
  return 0;
}
