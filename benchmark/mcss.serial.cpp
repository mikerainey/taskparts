#include <taskparts/benchmark.hpp>
#include "mcss.hpp"

int main() {
  size_t n = taskparts::cmdline::parse_or_default_long("n", 100000000);
  std::vector<long long> a;
  parlay::benchmark_taskparts([&] (auto sched) {
    mcss(a);
  }, [&] (auto sched) {
    a.resize(n);
    parlay::parallel_for(0, n, [&] (auto i) {
      a[i] = (i % 2 == 0 ? -1 : 1) * i;
    });
  });
  return 0;
}
