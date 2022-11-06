#include "common.hpp"
#include <parlay/primitives.h>
#include <parlay/sequence.h>
#include <parlay/random.h>
#include "delaunay.h"

parlay::random_generator gen;
std::uniform_real_distribution<real> dis;
parlay::sequence<point> points; 
parlay::sequence<tri> result;

auto gen_input() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  long n = std::max((size_t)1, (size_t)taskparts::cmdline::parse_or_default_long("n", 1 * 1000 * 1000));
  parlay::random_generator gen1(0);
  std::uniform_real_distribution<real> dis1(0.0,1.0);
  gen = gen1;
  dis = dis1;
  // generate n random points in a unit square
  points = parlay::tabulate(n, [] (point_id i) -> point {
    auto r = gen[i];
    return point{i, dis(r), dis(r)};});
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  result = delaunay(points);
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}

int main() {
  parlay::benchmark_taskparts([&] (auto sched) { // benchmark
    benchmark();
  }, [&] (auto sched) { // setup
    gen_input();
  }, [&] (auto sched) { // teardown
    std::cout << "number of triangles in the mesh = " << result.size() << std::endl;
  }, [&] (auto sched) { // reset

  });
  return 0;
}
