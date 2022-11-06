#include <iostream>

#include "cilk.hpp"

#include <parlay/io.h>
#include <parlay/primitives.h>
#include <parlay/sequence.h>
#include <parlay/internal/get_time.h>

#include "knuth_morris_pratt.h"
#include "common.hpp"

parlay::chars str;
parlay::chars search_str;
parlay::sequence<long> locations;

auto gen_input() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  auto input = taskparts::cmdline::parse_or_default_string("input", "rmat");
  str = parlay::chars_from_file(input.c_str());
  search_str = parlay::to_chars(taskparts::cmdline::parse_or_default_string("search_str", "foo").c_str());
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  locations = knuth_morris_pratt(str, search_str);
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
#ifndef NDEBUG
    std::cout << "total matches = " << locations.size() << std::endl;
    if (locations.size() > 10) {
      auto r = parlay::to_sequence(locations.cut(0,10));
      std::cout << "at locations: " << to_chars(r) << " ..." <<  std::endl;
    } else if (locations.size() > 0) {
      std::cout << "at locations: " << to_chars(locations) << std::endl;
    }
#endif
  }, [&] { // reset
    locations.clear();
  });
  return 0;
}
