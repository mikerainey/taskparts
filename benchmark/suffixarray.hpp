#pragma once

#include "common.hpp"
#include <common/IO.h>
#include <common/sequenceIO.h>
#include <benchmarks/suffixArray/bench/SA.h>
#include <benchmarks/suffixArray/parallelKS/SA.C>

using uchar = unsigned char;

parlay::sequence<uchar> ss;
parlay::sequence<indexT> R;

auto gen_input() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  std::string fname = taskparts::cmdline::parse_or_default_string("input", "chr22.dna");
  parlay::sequence<char> s = benchIO::readStringFromFile(fname.c_str());
  ss = parlay::tabulate(s.size(), [&] (size_t i) -> uchar {return (uchar) s[i];});
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  R = suffixArray(ss);
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}
