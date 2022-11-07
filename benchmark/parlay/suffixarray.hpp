#pragma once

#include "common.hpp"
#include <parlay/io.h>
#include <parlay/primitives.h>
#include <parlay/sequence.h>
#include <parlay/internal/get_time.h>

#include "suffix_array.h"

using uchar = unsigned char;
using charseq = parlay::sequence<char>;
using uint = unsigned int;

charseq str;
parlay::sequence<uint> result;

auto gen_input() {
  std::string infile_path = "";
  if (const auto env_p = std::getenv("TASKPARTS_BENCHMARK_INFILE_PATH")) {
    infile_path = std::string(env_p);
  }
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  std::string input = taskparts::cmdline::parse_or_default_string("input", "chr22.dna");
  auto infile = infile_path + "/" + input;
  str = parlay::chars_from_file(infile.c_str());
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  result = suffix_array(str);
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}
