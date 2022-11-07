#include "dedup.hpp"

int main() {
  parlay::benchmark_taskparts([&] (auto sched) { // benchmark
    benchmark();
  }, [&] (auto sched) { // setup
    gen_input();
  }, [&] (auto sched) { // teardown
    // later: write results to outfile
  }, [&] (auto sched) { // reset
    reset();    
  });
  return 0;
}
