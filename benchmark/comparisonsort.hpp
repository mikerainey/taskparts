#pragma once

#include "common.hpp"
#include <testData/sequenceData/sequenceData.h>
#include <common/sequenceIO.h>

using item_type = double;

parlay::sequence<item_type> a;
parlay::sequence<item_type> b;

auto gen_input() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  //  if (include_infile_load) {
    auto input = taskparts::cmdline::parse_or_default_string("input", "");
    if (input == "") {
      exit(-1);
    }
    auto infile = input + ".seq";
    auto tokens = benchIO::get_tokens(infile.c_str());
    a = benchIO::parseElements<item_type>(tokens.cut(1, tokens.size()));
    return;
    //  }
  size_t n = taskparts::cmdline::parse_or_default_long("n", 10000000);
  taskparts::cmdline::dispatcher d;
  d.add("random_double", [&] {
    a = dataGen::rand<item_type>((size_t) 0, n);
  });
  d.add("exponential_double", [&] { 
    a = dataGen::expDist<item_type>(0,n);
  });
  d.add("almost_sorted_double", [&] {
    size_t swaps = taskparts::cmdline::parse_or_default_long("swaps", floor(sqrt((float) n)));
    a = dataGen::almostSorted<double>(0, n, swaps);
  });
  d.dispatch_or_default("input", "random_double");
}

template <class T, class BinPred>
parlay::sequence<T> compSort2(parlay::sequence<T> &A, const BinPred& f);

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  b = compSort2(a, [] (item_type x, item_type y) { return x < y; });
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}
