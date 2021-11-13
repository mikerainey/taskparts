#pragma once

#include <parlay/delayed_sequence.h>
#include <parlay/monoid.h>
#include <parlay/primitives.h>
#include <parlay/parallel.h>
#include <common/IO.h>
#include <common/sequenceIO.h>
#ifndef PARLAY_SEQUENTIAL
#include <suffixArray/bench/SA.h>
#include <suffixArray/parallelKS/SA.C>
#else
#include <suffixArray/bench/SA.h>
#include <suffixArray/parallelKS/SA.C>
// later: get serial algorithm to compile
/*
#include <suffixArray/bench/SA.h>
#include <suffixArray/serialDivsufsort/SA.C>
#include <suffixArray/serialDivsufsort/divsufsort.C>
#include <suffixArray/serialDivsufsort/trsort.C>
#include <suffixArray/serialDivsufsort/sssort.C>
*/
#endif
#include "../example/fib_serial.hpp"

using uchar = unsigned char;

parlay::sequence<uchar> ss;
parlay::sequence<indexT> R;
bool include_infile_load;

auto load_input() {
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  std::string fname = taskparts::cmdline::parse_or_default_string("input", "chr22.dna");
  parlay::sequence<char> s = benchIO::readStringFromFile(fname.c_str());
  ss = parlay::tabulate(s.size(), [&] (size_t i) -> uchar {return (uchar) s[i];});
}

auto benchmark_no_repeat() {
  if (include_infile_load) {
    load_input();
  }
  R = suffixArray(ss);
}

auto benchmark_with_repeat(size_t repeat) {
  for (size_t i = 0; i < repeat; i++) {
    fib_serial(30);
    R = suffixArray(ss);    
  }
}

auto benchmark() {
  size_t repeat = taskparts::cmdline::parse_or_default_long("repeat", 0);
  if (repeat == 0) {
    benchmark_no_repeat();
  } else {
    benchmark_with_repeat(repeat);
  }
}
