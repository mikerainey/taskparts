#pragma once

#include <math.h>
#include "common.hpp"
#include <common/IO.h>
#include <common/geometry.h>
#include <common/geometryIO.h>
#include <testData/geometryData/geometryData.h>
#include <delaunayRefine/incrementalRefine/refine.C>
using namespace benchIO;
using namespace dataGen;
using namespace std;

using coord = double;
using point = point2d<coord>;

triangles<point> R;
triangles<point> Tri;

auto gen_input() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  auto input = taskparts::cmdline::parse_or_default_string("input", "");
  if (input == "") {
    exit(-1);
  }
  auto infile = input + ".tri";
  Tri = readTrianglesFromFile<point>(infile.c_str(),0);
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  R = refine(Tri);
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}

auto reset() {
  R.P.clear();
  R.T.clear();
}
