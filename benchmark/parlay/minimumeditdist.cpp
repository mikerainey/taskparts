#include <iostream>
#include <string>

#include <parlay/primitives.h>
#include <parlay/sequence.h>
#include <parlay/io.h>

#include "minimum_edit_distance.h"
#include "common.hpp"

long n, result;
parlay::sequence<long> s0, s1;

auto gen_input() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  n = std::max((size_t)1, (size_t)taskparts::cmdline::parse_or_default_long("n", 100 * 1000));
  // remove approximately 10% random elements from [0,...,n-1]
  // for two sequences
  // edit distance should be about .2*n
  auto gen0 = parlay::random_generator(0);
  auto gen1 = parlay::random_generator(1);
  std::uniform_int_distribution<long> dis(0,10);
  s0 = parlay::filter(parlay::iota(n), [&] (long i) {
    auto r = gen0[i]; return dis(r) != 0;});
  s1 = parlay::filter(parlay::iota(n), [&] (long i) {
    auto r = gen1[i]; return dis(r) != 0;});
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  result = minimum_edit_distance(s0, s1);
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
    std::cout << "total_ops " << s0.size() * s1.size() << std::endl;
    std::cout << "edit_dist " << result << std::endl;
  }, [&] (auto sched) { // reset
    result = 0;
  });
  return 0;
}
