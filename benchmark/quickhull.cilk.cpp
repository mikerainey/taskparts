#include "cilk.hpp"
#include "quickhull.hpp"

int main() {
  parlay::sequence<point2d<coord>> Points;
  parlay::sequence<indexT> result;
  taskparts::benchmark_cilk([&] {
    result = hull(Points);
  }, [&] {
    Points = gen_input();
  });
  return 0;
}
