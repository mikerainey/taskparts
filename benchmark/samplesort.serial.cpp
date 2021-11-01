#include <taskparts/benchmark.hpp>
#include "samplesort.hpp"

int main() {
  parlay::benchmark_taskparts([&] (auto sched) {
    compSort(a, [] (item_type x, item_type y) { return x < y; });
  }, [&] (auto sched) {
    gen_input();
  });
  return 0;
}
