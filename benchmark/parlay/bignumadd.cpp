#include <taskparts/benchmark.hpp>
#include "bignumadd.hpp"

int main() {
  parlay::benchmark_taskparts([&] (auto sched) { // benchmark
    benchmark();
  }, [&] (auto sched) { // setup
    gen_input();
  }, [&] (auto sched) { // teardown
    std::cout << "result " << result.size() << std::endl;
  }, [&] (auto sched) { // reset
    result.clear();
  });
  return 0;
}
