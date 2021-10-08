#include "cilk.hpp"
#include "samplesort.hpp"

int main() {
  size_t n = taskparts::cmdline::parse_or_default_long("n", dflt_n);
  size_t r = taskparts::cmdline::parse_or_default_long("r", n);

  parlay::sequence<long> a;
  taskparts::benchmark_cilk([&] {
    compSort(a, [] (long x, long y) { return x < y; });
  }, [&] {
    a = dataGen::randIntRange<long>((size_t) 0,n,r);
  });
  return 0;
}