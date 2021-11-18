#pragma once

#include "common.hpp"
#include <testData/sequenceData/sequenceData.h>
#include <common/sequenceIO.h>
//#ifndef PARLAY_SEQUENTIAL
#include <histogram/parallel/histogram.C>
//#else
//#include <histogram/sequential/histogram.C>
//#endif

using namespace parlay;
using namespace benchIO;

sequence<uint> In;
sequence<uint> R;
uint buckets;

auto gen_input() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  auto input = taskparts::cmdline::parse_or_default_string("input", "random_int");
  if (input == "") {
    exit(-1);
  }
  auto infile = input + ".seq";
  In = readIntSeqFromFile<uint>(infile.c_str());
  if (input == "random_int") {
    buckets = In.size();
  } else if (input == "random_256_int"){
    buckets = 256;
  } else {
    buckets = taskparts::cmdline::parse_or_default_int("buckets", In.size());
  }
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  R = histogram(In, buckets);
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
