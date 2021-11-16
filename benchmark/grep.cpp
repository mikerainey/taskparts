#include <taskparts/benchmark.hpp>
#include "grep.hpp"

int main() {
  parlay::benchmark_taskparts([&] (auto sched) { // benchmark
    benchmark();
  }, [&] (auto sched) { // setup
    gen_input();
  }, [&] (auto sched) { // teardown
    std::cout << "result " << result << std::endl;
  }, [&] (auto sched) { // reset

  });
  return 0;
}

