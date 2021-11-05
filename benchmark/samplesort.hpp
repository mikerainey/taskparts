#pragma once

#include <parlay/delayed_sequence.h>
#include <parlay/monoid.h>
#include <parlay/primitives.h>
#include <parlay/parallel.h>
#include <testData/sequenceData/sequenceData.h>
#include <common/sequenceIO.h>
#ifndef PARLAY_SEQUENTIAL
#include <comparisonSort/sampleSort/sort.h>
#else
#include <comparisonSort/serialSort/sort.h>
#endif

using item_type = double;

parlay::sequence<item_type> a;
bool include_infile_load;

auto gen_input() {
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  if (include_infile_load) {
    auto input = taskparts::cmdline::parse_or_default_string("input", "");
    if (input == "") {
      exit(-1);
    }
    auto infile = input + ".seq";
    auto tokens = benchIO::get_tokens(infile.c_str());
    a = benchIO::parseElements<item_type>(tokens.cut(1, tokens.size()));
    return;
  }
  size_t n = taskparts::cmdline::parse_or_default_long("n", 10000000);
  taskparts::cmdline::dispatcher d;
  d.add("random", [&] {
    a = dataGen::rand<item_type>((size_t) 0, n);
  });
  d.add("exponential", [&] { 
    a = dataGen::expDist<item_type>(0,n);
  });
  d.add("almost_sorted", [&] {
    size_t swaps = taskparts::cmdline::parse_or_default_long("swaps", floor(sqrt((float) n)));
    a = dataGen::almostSorted<double>(0, n, swaps);
  });
  d.dispatch_or_default("input", "random");
}

auto benchmark_no_swaps() {
  if (include_infile_load) {
    gen_input();
  }
  compSort(a, [] (item_type x, item_type y) { return x < y; });
}

auto benchmark_with_swaps(size_t repeat) {
  size_t repeat_swaps = taskparts::cmdline::parse_or_default_long("repeat_swaps", 1000000);
  auto n = a.size();
  for (size_t i = 0; i < repeat; i++) {
    for (size_t j = 0; j < repeat_swaps; j++) {
      auto k1 = taskparts::hash(i + j) % n;
      auto k2 = taskparts::hash(k1 + i + j) % n;
      std::swap(a[k1], a[k2]);
    }
    compSort(a, [] (item_type x, item_type y) { return x < y; });
  }
}
