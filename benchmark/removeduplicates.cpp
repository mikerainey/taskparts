#include <taskparts/benchmark.hpp>
#include "removeduplicates.hpp"

int main() {
  size_t n = taskparts::cmdline::parse_or_default_long("n", dflt_n);
  size_t r = taskparts::cmdline::parse_or_default_long("r", n);
  
  parlay::sequence<int> a;
  parlay::sequence<int> res;
  parlay::benchmark_taskparts([&] (auto sched) {
    res = dedup(a);
  }, [&] (auto sched) {
    a = dataGen::randIntRange<int>((size_t) 0,n,r);
  });
  return 0;
}
