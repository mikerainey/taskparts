#include <iostream>
#include <string>
#include <random>

#include <parlay/primitives.h>
#include <parlay/random.h>
#include <parlay/internal/get_time.h>

#include "quickhull.h"
#include "common.hpp"

long n;
parlay::sequence<point> points;
intseq results;

auto gen_input() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  n = std::max((size_t)1, (size_t)taskparts::cmdline::parse_or_default_long("n", 50 * 1000 * 1000));
  parlay::random_generator gen(0);
  std::uniform_real_distribution<> dis(0.0,1.0);
  // generate n random points in a unit square
  points = parlay::tabulate(n, [&] (long i) -> point {
    auto r = gen[i];
    return point{dis(r), dis(r)};});
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  results = upper_hull(points);
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
    std::cout << "nb_upper_hull = " << results.size() << std::endl;
  }, [&] (auto sched) { // reset
    results.clear();
  });
  return 0;
}
