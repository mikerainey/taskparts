#pragma once

#include "common.hpp"
#include <common/IO.h>
#include <common/graph.h>
#include <common/graphIO.h>
#ifndef PARLAY_SEQUENTIAL
#include <benchmarks/maximalMatching/serialMatching/matching.C>
#else
#include <benchmarks/maximalMatching/incrementalMatching/matching.C>
#endif

using namespace std;
using namespace benchIO;

edges E;
parlay::sequence<edgeId> edgeIds;

auto gen_input() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  auto input = taskparts::cmdline::parse_or_default_string("input", "");
  if (input == "") {
    exit(-1);
  }
  auto infile = input + ".edg";
  E = readEdgeArrayFromFile<vertexId>((char*)infile.c_str());
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  edgeIds = maximalMatching(E);
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}

auto reset() {
  edgeIds.clear();
}
