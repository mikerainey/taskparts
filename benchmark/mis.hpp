#pragma once

#include <parlay/delayed_sequence.h>
#include <parlay/monoid.h>
#include <parlay/primitives.h>
#include <parlay/parallel.h>

#include <common/IO.h>
#include <common/graph.h>
#include <common/graphIO.h>
#ifndef PARLAY_SEQUENTIAL
#include <maximalIndependentSet/serialMIS/MIS.C>
#else
#include <maximalIndependentSet/incrementalMIS/MIS.C>
#endif
#include <testData/graphData/rMat.h>
#include "../example/fib_serial.hpp"

using namespace std;
using namespace benchIO;

Graph G;
parlay::sequence<char> flags;
bool include_infile_load;

auto gen_input() {
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  // note: not generating input graphs because it's too slow
  if (true /*include_infile_load*/) {
    auto input = taskparts::cmdline::parse_or_default_string("input", "");
    if (input == "") {
      exit(-1);
    }
    auto infile = input + ".adj";
    G = readGraphFromFile<vertexId,edgeId>((char*)infile.c_str());
    return;
  }
  taskparts::cmdline::dispatcher d;
  d.add("rmat", [&] {
    size_t n = taskparts::cmdline::parse_or_default_long("n", 5000000);
    double a = 0.5;
    double b = 0.1;
    double c = b;
    size_t m = 10*n;
    size_t seed = 1;
    auto EA = edgeRmat<vertexId>(n, m, seed, a, b, c);
    G = graphFromEdges<vertexId,edgeId>(EA, true);    
  });
  d.dispatch_or_default("input", "rmat");
}

auto benchmark_no_swaps() {
  if (include_infile_load) {
    gen_input();
  }
  flags = maximalIndependentSet(G);
}

auto benchmark_with_swaps(size_t repeat) {
  size_t repeat_swaps = taskparts::cmdline::parse_or_default_long("repeat_swaps", 1000000);
  for (size_t i = 0; i < repeat; i++) {
    fib_serial(30);
    flags = maximalIndependentSet(G);    
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
  flags.clear();
}
