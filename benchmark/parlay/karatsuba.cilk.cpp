#include <iostream>
#include <string>
#include <random>
#include <limits>

#include "cilk.hpp"

#include <parlay/primitives.h>
#include <parlay/random.h>

#include "karatsuba.h"
#include "common.hpp"

bigint a, b, result;

auto gen_input() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  size_t n = std::max((size_t)1, (size_t)taskparts::cmdline::parse_or_default_long("n", 20 * 1000 * 1000));
  long m = n/digit_len;
  auto randnum = [] (long m, long seed) {
    parlay::random_generator gen(seed);
    auto maxv = std::numeric_limits<digit>::max();
    std::uniform_int_distribution<digit> dis(0, maxv);
    return parlay::tabulate(m, [&] (long i) {
      auto r = gen[i];
      if (i == m-1) return dis(r)/2; // to ensure it is not negative
      else return dis(r);});
  };
  a = randnum(m, 0);
  b = randnum(m, 1);
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  result = karatsuba(a, b);
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}

int main() {
  taskparts::benchmark_cilk([&] {
    benchmark();
  }, [&] { // setup
    gen_input();
  }, [&] { // teardown
    std::cout << "result " << result.size() << std::endl;
  }, [&] { // reset
    result.clear();
  });
  return 0;
}
