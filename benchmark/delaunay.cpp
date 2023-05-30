#include <iostream>
#include <string>
#include <random>

#include <parlay/primitives.h>
#include <parlay/sequence.h>
#include <parlay/random.h>

#include "delaunay.h"

#include "benchmark.hpp"

// **************************************************************
// Driver
// **************************************************************
int main(int argc, char* argv[]) {
  auto usage = "Usage: delaunay <n>";
  if (argc != 2) std::cout << usage << std::endl;
  else {
    point_id n;
    try {n = std::stoi(argv[1]);}
    catch (...) {std::cout << usage << std::endl; return 1;}
    parlay::random_generator gen(0);
    std::uniform_real_distribution<real> dis(0.0,1.0);

    // generate n random points in a unit square
    auto points = parlay::tabulate(n, [&] (point_id i) -> point {
      auto r = gen[i];
      return point{i, dis(r), dis(r)};});

    parlay::sequence<tri> result;
    taskparts::benchmark([&] {
      result = delaunay(points);
    });
    std::cout << "number of triangles in the mesh = " << result.size() << std::endl;
  }
}
