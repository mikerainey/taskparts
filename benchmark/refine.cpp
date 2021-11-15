#include <taskparts/benchmark.hpp>
#include "refine.hpp"

int main() {
  parlay::benchmark_taskparts([&] (auto sched) {
    benchmark();
  }, [&] (auto sched) {
    gen_input();
  }, [&] (auto sched) { // teardown
    // later: write results to outfile
  }, [&] (auto sched) { // reset
    reset();    
  });
  return 0;
}
