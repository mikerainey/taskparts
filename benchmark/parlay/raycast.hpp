#pragma once

#include <math.h>
#include "common.hpp"
#include <common/IO.h>
#include <common/geometry.h>
#include <common/geometryIO.h>
#include <testData/geometryData/geometryData.h>
#include <benchmarks/rayCast/kdTree/ray.C>

using namespace benchIO;
using namespace dataGen;
using namespace std;

triangles<point> T;
parlay::sequence<ray<point>> rays;
parlay::sequence<index_t> R;

auto gen_input() {
  std::string infile_path = "";
  if (const auto env_p = std::getenv("TASKPARTS_BENCHMARK_INFILE_PATH")) {
    infile_path = std::string(env_p);
  }
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  auto input = taskparts::cmdline::parse_or_default_string("input", "happy");
  if (input == "") {
    exit(-1);
  }
  auto infile_rays = infile_path + "/" + input + "Rays";
  auto infile_tris = infile_path + "/" + input + "Triangles";
  T = readTrianglesFromFile<point>(infile_tris.c_str(), 1);
  parlay::sequence<point> Pts = readPointsFromFile<point>(infile_rays.c_str());
  size_t n = Pts.size()/2;
  rays = parlay::tabulate(n, [&] (size_t i) -> ray<point> {
      return ray<point>(Pts[2*i], Pts[2*i+1]-point(0,0,0));});
}


auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  R = rayCast(T, rays);
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}

auto reset() {
  R.clear();
}
