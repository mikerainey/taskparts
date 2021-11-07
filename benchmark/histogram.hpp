#pragma once

#include <parlay/delayed_sequence.h>
#include <parlay/monoid.h>
#include <parlay/primitives.h>
#include <parlay/parallel.h>
#include <testData/sequenceData/sequenceData.h>
#include <common/sequenceIO.h>
#ifndef PARLAY_SEQUENTIAL
#include <histogram/parallel/histogram.C>
#else
#include <histogram/sequential/histogram.C>
#endif
#include "../example/fib_serial.hpp"

using namespace parlay;
using namespace benchIO;

sequence<uint> In;
sequence<uint> R;
uint buckets;
bool include_infile_load;

auto gen_input() {
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  auto input = taskparts::cmdline::parse_or_default_string("input", "");
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

auto benchmark_no_swaps() {
  if (include_infile_load) {
    gen_input();
  }
  R = histogram(In, buckets);
}

auto benchmark_with_swaps(size_t repeat) {
  size_t repeat_swaps = taskparts::cmdline::parse_or_default_long("repeat_swaps", 1000000);
  for (size_t i = 0; i < repeat; i++) {
    fib_serial(30);
    R = histogram(In, buckets);
  }
}

auto benchmark() {
  size_t repeat = taskparts::cmdline::parse_or_default_long("repeat", 0);
  if (repeat == 0) {
    benchmark_no_swaps();
  } else {
    benchmark_with_swaps(repeat);
  }
}

auto reset() {
  R.clear();
}
