#pragma once

#include <parlay/delayed_sequence.h>
#include <parlay/monoid.h>
#include <parlay/primitives.h>
#include <parlay/parallel.h>
#include <testData/sequenceData/sequenceData.h>
#ifndef PARLAY_SEQUENTIAL
#include <comparisonSort/sampleSort/sort.h>
#else
#include <comparisonSort/serialSort/sort.h>
#endif

using item_type = double;

parlay::sequence<item_type> a;

auto gen_input() {
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
