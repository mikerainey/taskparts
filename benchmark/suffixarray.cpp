#include <taskparts/benchmark.hpp>
#include "suffixarray.hpp"

int main() {
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  parlay::benchmark_taskparts([&] (auto sched) {
    benchmark();
  }, [&] (auto sched) {
    if (! include_infile_load) {
      load_input();
    }
  });
  return 0;
}
