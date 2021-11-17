#include <taskparts/benchmark.hpp>
#include "mcss.hpp"

int main() {
  parlay::benchmark_taskparts([&] (auto sched) { // benchmark
    benchmark();
  }, [&] (auto sched) { // setup
    gen_input();
  }, [&] (auto sched) { // teardown
    printf("result %f\n", result);
  });
  return 0;
}
