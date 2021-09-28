#include <math.h>
#include <taskparts/benchmark.hpp>
#include <parlay/delayed_sequence.h>
#include <parlay/monoid.h>
#include <parlay/primitives.h>
#include <parlay/parallel.h>
#include <common/IO.h>
#include <common/geometry.h>
#include <common/geometryIO.h>
#include <testData/geometryData/geometryData.h>
#include <convexHull/quickHull/hull.C>
using namespace benchIO;
using namespace dataGen;
using namespace std;

using coord = double;

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

int main() {
  size_t n = taskparts::cmdline::parse_or_default_long("n", 1000000);
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
