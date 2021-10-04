#include <taskparts/benchmark.hpp>
#include "quickhull.hpp"

int main() {
  size_t n = taskparts::cmdline::parse_or_default_long("n", dflt_n);
  parlay::sequence<point2d<coord>> Points;
  int dims = taskparts::cmdline::parse_or_default_int("dims", 2);
  bool inSphere = taskparts::cmdline::parse_or_default_bool("in_sphere", true);
  bool onSphere = taskparts::cmdline::parse_or_default_bool("on_sphere", false);
  bool plummerOrKuzmin = taskparts::cmdline::parse_or_default_bool("plummer_or_kuzmin", true);
  parlay::sequence<indexT> result;
  parlay::benchmark_taskparts([&] (auto sched) {
    result = hull(Points);
  }, [&] (auto sched) {
    Points = parlay::tabulate(n, [&] (size_t i) -> point2d<coord> {
	if (inSphere) return randInUnitSphere2d<coord>(i);
	else if (onSphere) return randOnUnitSphere2d<coord>(i);
	else if (plummerOrKuzmin) return randKuzmin<coord>(i);
	else return rand2d<coord>(i);
      });
  });
  return 0;
}
