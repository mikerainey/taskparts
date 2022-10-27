#pragma once

#include "common.hpp"
#include "bigint_add.h"

bigint a, b, result;

auto gen_input() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  size_t n = std::max((size_t)1, (size_t)taskparts::cmdline::parse_or_default_long("n", 9000l * 1000l * 1000l));
  auto rand_num = [] (long m, long seed) {
    parlay::random_generator gen(seed);
    auto maxv = std::numeric_limits<digit>::max();
    std::uniform_int_distribution<digit> dis(0, maxv);
    return parlay::tabulate(m, [&] (long i) {
      auto r = gen[i];
      return dis(r);});
  };
  long m = n/digit_len;
  a = rand_num(m, 0);
  b = rand_num(m, 1);
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  result = add(a, b);
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}
