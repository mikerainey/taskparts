#include "cilk.hpp"
#include <iostream>
#include <string>
#include <random>

#include "parlay/primitives.h"
#include "parlay/random.h"
#include "parlay/io.h"

#include "knn.h"
#include "common.hpp"

long n;
int k = 10;
knn_graph r;
parlay::sequence<coords> _points;

// checks 10 random points and returns the number of points with errors
long check(const parlay::sequence<coords>& points, const knn_graph& G, int k) {
  long n = points.size();
  long num_trials = std::min<long>(20, points.size());
  parlay::random_generator gen(27);
  std::uniform_int_distribution<long> dis(0, n-1);
  auto distance_sq = [] (const coords& a, const coords& b) {
    double r = 0.0;
    for (int i = 0; i < dims; i++) {
      double diff = (a[i] - b[i]);
      r += diff*diff; }
    return r; };
  return parlay::reduce(parlay::tabulate(num_trials, [&] (long a) -> long {
    auto r = gen[a];
    idx i = dis(r);
    coords p = points[i];
    auto x = parlay::to_sequence(parlay::sort(parlay::map(points, [&] (auto q) {
      return distance_sq(p, q);})).cut(1,k+1));
    auto y = parlay::reverse(parlay::map(G[i], [&] (long j) {
      return distance_sq(p,points[j]);}));
    return y != x;}));
}

auto gen_input() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  n = std::max((size_t)1, (size_t)taskparts::cmdline::parse_or_default_long("n", 10 * 1000 * 1000));
  parlay::random_generator gen(0);
  std::uniform_int_distribution<coord> dis(0,1000000000);
  // generate n random points in a unit square
  _points = parlay::tabulate(n, [&] (long i) {
    auto r = gen[i];
    coords pnt;
    for (coord& c : pnt) c = dis(r);
    return pnt;
  });
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  r = build_knn_graph(_points, k);
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}

int main() {
  taskparts::benchmark_cilk([&] {
    benchmark();
  }, [&] { // setup
    gen_input();
  }, [&] { // teardown
#ifndef NDEBUG
    if (check(_points, r, k) > 0)
      std::cout << "found error" << std::endl;
    else
      std::cout << "generated " << k << " nearest neighbor graph for " << r.size()
                << " points." << std::endl;
#endif
  }, [&] { // reset
    r.clear();
  });
  return 0;
}
