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
  std::string infile_path = "";
  if (const auto env_p = std::getenv("TASKPARTS_BENCHMARK_INFILE_PATH")) {
    infile_path = std::string(env_p);
  }
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  int n = std::max((size_t)1, (size_t)taskparts::cmdline::parse_or_default_long("n", 40));
  auto input = taskparts::cmdline::parse_or_default_string("input", "chr22.dna");
  auto infile = infile_path + "/" + input;
  str = parlay::chars_from_file(infile.c_str());
  auto input_search = taskparts::cmdline::parse_or_default_string("search", "foo");
  search_str = parlay::to_chars(input_search.c_str());
  auto str2 = str;
  for (int i = 0; i < n; i++) {  
    str.append(str2);
  }
  str2.clear();
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
