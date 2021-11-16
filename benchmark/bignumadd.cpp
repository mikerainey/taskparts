#include <taskparts/benchmark.hpp>
#include "bignumadd.hpp"

int main() {
  parlay::benchmark_taskparts([&] (auto sched) { // benchmark
    benchmark();
  }, [&] (auto sched) { // setup
    gen_input();
  }, [&] (auto sched) { // teardown
    std::cout << "result1 " << result1 << std::endl;
    std::cout << "result2 " << result2 << std::endl;
  }, [&] (auto sched) { // reset

  });
  return 0;
}
