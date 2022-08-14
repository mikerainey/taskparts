#include <iostream>
#include <string>
#include <random>
#include <limits>

#include <parlay/io.h>
#include <parlay/primitives.h>
#include <parlay/random.h>

#include "tokens.h"
#include "common.hpp"

parlay::chars str;
parlay::sequence<parlay::chars> r;

auto gen_input() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  auto input = taskparts::cmdline::parse_or_default_string("input", "tokens.txt");
  str = parlay::chars_from_file(input.c_str());
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  r = tokens(str, [&] (char c) { return c == ' '; });
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
    std::cout << "nb_tokens " << r.size() << std::endl;
  }, [&] (auto sched) { // reset
    r.clear();
  });
  return 0;
}
