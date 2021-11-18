#pragma once

#include "common.hpp"
#include <testData/sequenceData/sequenceData.h>
#include <common/sequenceIO.h>
//#ifndef PARLAY_SEQUENTIAL
#include <index/parallel/index.C>
//#else
//#include <index/sequential/index.C>
//#endif

charseq R;
charseq s;
charseq start;
bool verbose = false;

auto gen_input() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  auto iFile = taskparts::cmdline::parse_or_default_string("input", "wikisamp.xml");
  s = parlay::to_sequence(parlay::file_map(iFile));
  string header = "<doc";
  start = parlay::to_sequence(header);
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  R = build_index(s, start, verbose);
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}

auto reset() {
  R.clear();
}
