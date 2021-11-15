#pragma once

#include <math.h>
#include "common.hpp"
#include <common/IO.h>
#include <common/geometry.h>
#include <common/geometryIO.h>
#include <testData/geometryData/geometryData.h>
#include <nearestNeighbors/octTree/neighbors.h>

using namespace benchIO;
using namespace dataGen;
using namespace std;

using coord = double;
using point2 = point2d<coord>;
using point3 = point3d<coord>;

template <class PT, int KK>
struct vertex {
  using pointT = PT;
  int identifier;
  pointT pt;         // the point itself
  vertex* ngh[KK];    // the list of neighbors
  vertex(pointT p, int id) : pt(p), identifier(id) {}
  size_t counter;
};

int k;
int d;
parlay::sequence<point2> PIn2;
parlay::sequence<point3> PIn3;

auto gen_input() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  auto input = taskparts::cmdline::parse_or_default_string("input", "");
  if (input == "") {
    exit(-1);
  }
  d = taskparts::cmdline::parse_or_default_int("d", 2);
  k = taskparts::cmdline::parse_or_default_int("k", 1);
  auto infile = input + ".pts";
  auto iFile = infile.c_str();
  if (d == 2) {
    PIn2 = readPointsFromFile<point2>(iFile);
  } else if (d == 3) {
    PIn3 = readPointsFromFile<point3>(iFile);
  }
}

template <int maxK, class point>
void timeNeighbors(parlay::sequence<point> &pts, int k) {
  size_t n = pts.size();
  using vtx = vertex<point,maxK>;
  int dimensions = pts[0].dimension();
  auto vv = parlay::tabulate(n, [&] (size_t i) -> vtx {
      return vtx(pts[i],i);
    });
  auto v = parlay::tabulate(n, [&] (size_t i) -> vtx* {
      return &vv[i];});
  ANN<maxK>(v, k);
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  if (d == 2) {
    if (k == 1) timeNeighbors<1>(PIn2, 1);
    else timeNeighbors<100>(PIn2, k);
  } else if (d == 3) {
    if (k == 1) timeNeighbors<1>(PIn3, 1);
    else timeNeighbors<100>(PIn3, k);
  }
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}
