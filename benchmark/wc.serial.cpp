#include <taskparts/benchmark.hpp>
#include "wc.hpp"

int main() {
  size_t n = taskparts::cmdline::parse_or_default_long("n", 100000000);
  std::string s;
  parlay::benchmark_taskparts([&] (auto sched) {
    wc(s);
  }, [&] (auto sched) {
    s.resize(n, 'b');
  });
  return 0;
}
