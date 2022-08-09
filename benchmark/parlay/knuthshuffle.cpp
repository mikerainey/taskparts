#include <iostream>
#include <string>

#include <parlay/primitives.h>
#include <parlay/sequence.h>
#include <parlay/io.h>

#include "knuth_shuffle.h"
#include "common.hpp"

long n;
parlay::sequence<long> result;
parlay::sequence<long> data;

auto gen_input() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  n = std::max((size_t)1, (size_t)taskparts::cmdline::parse_or_default_long("n", 1 * 1000 * 1000));
  data = parlay::tabulate(n, [&] (long i) {return i;});
  result = data;
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  random_shuffle(result);
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
#ifndef NDEBUG
    auto first_ten = result.head(std::min(n, 10l));
    std::cout << "first 10 elements: " << parlay::to_chars(first_ten) << std::endl;
#endif
  }, [&] (auto sched) { // reset
    result = data;
    data.clear();
  });
  return 0;
}
