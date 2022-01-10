#pragma once

#include <math.h>
#include "common.hpp"
#include <common/IO.h>
#include <common/geometry.h>
#include <common/geometryIO.h>
#include <testData/geometryData/geometryData.h>
#include <benchmarks/nBody/parallelCK/nbody.C>

using namespace benchIO;
using namespace dataGen;
using namespace std;

parlay::sequence<particle*> p;

auto gen_input() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  auto input = taskparts::cmdline::parse_or_default_string("input", "");
  if (input == "") {
    exit(-1);
  }
  auto infile = input + ".pts";
  auto iFile = infile.c_str();
  auto pts = readPointsFromFile<point>(iFile);
  auto pp = parlay::map(pts, [] (point p) -> particle {return particle(p, 1.0);});
  p = parlay::tabulate(pts.size(), [&] (size_t i) -> particle* {
      return &pp[i];});
}


auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  nbody(p);
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}
