#pragma once

#include <math.h>
#include <parlay/delayed_sequence.h>
#include <parlay/monoid.h>
#include <parlay/primitives.h>
#include <parlay/parallel.h>
#include <common/IO.h>
#include <common/geometry.h>
#include <common/geometryIO.h>
#include <testData/geometryData/geometryData.h>
#ifndef PARLAY_SEQUENTIAL
#include <convexHull/quickHull/hull.C>
#else
#include <convexHull/serialHull/hull.C>
#endif
#include <taskparts/cmdline.hpp>
using namespace benchIO;
using namespace dataGen;
using namespace std;

using coord = double;
parlay::sequence<point2d<coord>> Points;
parlay::sequence<indexT> result;
size_t dflt_n = 10000000;
bool include_infile_load;

template <class coord>
point2d<coord> randKuzmin(size_t i) {
  vector2d<coord> v = vector2d<coord>(randOnUnitSphere2d<coord>(i));
  size_t j = dataGen::hash<size_t>(i);
  double s = dataGen::hash<double>(j);
  double r = sqrt(1.0/((1.0-s)*(1.0-s))-1.0);
  return point2d<coord>(v*r);
}

template <class coord>
point3d<coord> randPlummer(size_t i) {
  vector3d<coord> v = vector3d<coord>(randOnUnitSphere3d<coord>(i));
  size_t j = dataGen::hash<size_t>(i);
  double s = pow(dataGen::hash<double>(j),2.0/3.0);
  double r = sqrt(s/(1-s));
  return point3d<coord>(v*r);
}

auto gen_input() {
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  if (include_infile_load) {
    auto input = taskparts::cmdline::parse_or_default_string("input", "");
    if (input == "") {
      exit(-1);
    }
    auto infile = input + ".geom";
    Points = benchIO::readPointsFromFile<point2d<coord>>(infile.c_str());
    return;
  }
  size_t n = taskparts::cmdline::parse_or_default_long("n", dflt_n);
  int dims = taskparts::cmdline::parse_or_default_int("dims", 2);
  bool inSphere = false;
  bool onSphere = false;
  bool kuzmin = false;
  taskparts::cmdline::dispatcher d;
  d.add("in_sphere", [&] { inSphere = true; });
  d.add("on_sphere", [&] { onSphere = true; });
  d.add("kuzmin", [&] { kuzmin = true; });
  d.dispatch_or_default("input", "in_sphere");
  Points = parlay::tabulate(n, [&] (size_t i) -> point2d<coord> {
      if (inSphere) return randInUnitSphere2d<coord>(i);
      else if (onSphere) return randOnUnitSphere2d<coord>(i);
      else if (kuzmin) return randKuzmin<coord>(i);
      else return rand2d<coord>(i);
    });
}

auto benchmark_no_nudges() {
  if (include_infile_load) {
    gen_input();
  }
  result = hull(Points);
}

auto benchmark_with_nudges(size_t repeat) {
  auto n = Points.size();
  size_t nudges = taskparts::cmdline::parse_or_default_long("nudges", n/2);
  for (size_t i = 0; i < repeat; i++) {
    for (size_t j = 0; j < nudges; j++) {
      auto k = taskparts::hash(i + j) % n;
      auto d = taskparts::hash(k + i + j) % 2;
      if (d == 0) {
	Points[k].x += ((coord)k) / (double)n;
      } else {
	Points[k].y += ((coord)k) / (double)n;
      }
    }
    result = hull(Points);
  }
}

auto benchmark() {
  size_t repeat = taskparts::cmdline::parse_or_default_long("repeat", 0);
  if (repeat == 0) {
    benchmark_no_nudges();
  } else {
    benchmark_with_nudges(repeat);
  }
}
