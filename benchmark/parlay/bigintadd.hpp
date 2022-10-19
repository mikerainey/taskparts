#pragma once

#include "common.hpp"
#include "bigint_add.h"

bigint a, b, result;

auto gen_input() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  size_t n = std::max((size_t)1, (size_t)taskparts::cmdline::parse_or_default_long("n", 5000 * 1000 * 1000));
  parlay::random_generator gen;
  std::uniform_int_distribution<unsigned int> dis(0,std::numeric_limits<int>::max());
  long m = n/32;
  a = parlay::tabulate(m, [&] (long i) {
    auto r = gen[i]; return dis(r);});
  b = parlay::tabulate(m, [&] (long i) {
    auto r = gen[i+n]; return dis(r);});
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
