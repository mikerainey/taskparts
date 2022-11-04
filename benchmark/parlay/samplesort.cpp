#include <iostream>
#include <string>
#include <random>

#include <parlay/io.h>
#include <parlay/primitives.h>
#include <parlay/random.h>
#include <parlay/sequence.h>

#include "samplesort.h"
#include "common.hpp"

long n;
parlay::sequence<long> data;
parlay::sequence<long> result;

auto gen_input() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  n = std::max((size_t)1, (size_t)taskparts::cmdline::parse_or_default_long("n", 50 * 1000 * 1000));
  parlay::random_generator gen;
  std::uniform_int_distribution<long> dis(0, n-1);
  // generate random long values
  data = parlay::tabulate(n, [&] (long i) {
    auto r = gen[i];
    return dis(r);});
  result = data;
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  sample_sort(result);
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
    auto first_ten = result.head(10);
    std::cout << "first 10 elements: " << parlay::to_chars(first_ten) << std::endl;
#endif
  }, [&] (auto sched) { // reset
    result = data;
  });
  return 0;
}
