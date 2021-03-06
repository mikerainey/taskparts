#include <taskparts/benchmark.hpp>
#include "integrate.hpp"

int main() {
  parlay::benchmark_taskparts([&] (auto sched) { // benchmark
    benchmark();
  }, [&] (auto sched) { // setup
    gen_input();
  });
  return 0;
}
