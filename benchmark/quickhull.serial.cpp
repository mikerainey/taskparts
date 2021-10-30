#include <taskparts/benchmark.hpp>
#include "quickhull.hpp"

int main() {
  parlay::sequence<point2d<coord>> Points;
  parlay::sequence<indexT> result;
  parlay::benchmark_taskparts([&] (auto sched) {
    result = hull(Points);
  }, [&] (auto sched) {
    Points = gen_input();
  });
  return 0;
}
