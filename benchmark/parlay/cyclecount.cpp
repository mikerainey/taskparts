#include "common.hpp"
#include "cycle_count.h"
#include <parlay/random.h>

parlay::sequence<long> permutation;
long count;

auto gen_input() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  long n = std::max((size_t)1, (size_t)taskparts::cmdline::parse_or_default_long("n", 10 * 1000 * 1000));
  permutation = parlay::random_permutation(n);
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  count = cycle_count(permutation);
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}

int main() {
  parlay::benchmark_taskparts([&] (auto sched) { // benchmark
    benchmark();
  }, [&] (auto sched) { // setup
    gen_input();
  }, [&] (auto sched) { // teardown
    std::cout << "nb_cycles " << count << std::endl;
  }, [&] (auto sched) { // reset

  });
  return 0;
}
