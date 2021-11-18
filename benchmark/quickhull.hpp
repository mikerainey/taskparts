#pragma once

#include <math.h>
#include "common.hpp"
#include <common/IO.h>
#include <common/geometry.h>
#include <common/geometryIO.h>
#include <testData/geometryData/geometryData.h>
//#ifndef PARLAY_SEQUENTIAL
#include <convexHull/quickHull/hull.C>
//#else
//#include <convexHull/serialHull/hull.C>
//#endif
using namespace benchIO;
using namespace dataGen;
using namespace std;

using coord = double;
parlay::sequence<point2d<coord>> Points;
parlay::sequence<indexT> result;
size_t dflt_n = 10000000;

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
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  //  if (include_infile_load) {
    auto input = taskparts::cmdline::parse_or_default_string("input", "kuzmin");
    if (input == "") {
      exit(-1);
    }
    auto infile = input + ".geom";
    Points = benchIO::readPointsFromFile<point2d<coord>>(infile.c_str());
    return;
    //  }
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

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  result = hull(Points);
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}
