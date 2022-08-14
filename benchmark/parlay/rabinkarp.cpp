#include <iostream>
#include <string>
#include <random>
#include <limits>

#include <parlay/io.h>
#include <parlay/primitives.h>
#include <parlay/random.h>

#include "rabin_karp.h"
#include "common.hpp"

parlay::chars str;
parlay::chars search_str;
parlay::sequence<long> locations;

auto gen_input() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  auto input = taskparts::cmdline::parse_or_default_string("input", "tokens.txt");
  str = parlay::chars_from_file(input.c_str());
  auto input_search = taskparts::cmdline::parse_or_default_string("search", "foo");
  search_str = parlay::to_chars(input_search.c_str());
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  locations = rabin_karp(str, search_str);
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
    std::cout << "matches " << locations.size() << std::endl;
    if (locations.size() > 10) {
      auto r = parlay::to_sequence(locations.cut(0,10));
      std::cout << "at_locations " << to_chars(r) << " ..." <<  std::endl;
    } else if (locations.size() > 0) {
      std::cout << "at_locations " << to_chars(locations) << std::endl;
    }
  }, [&] (auto sched) { // reset

  });
  return 0;
}
