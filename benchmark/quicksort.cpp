#include <taskparts/benchmark.hpp>
#include "quicksort.hpp"

int main() {
  parlay::benchmark_taskparts([&] (auto sched) {
    benchmark();
  }, [&] (auto sched) {
    gen_input();
  });
  return 0;
}
