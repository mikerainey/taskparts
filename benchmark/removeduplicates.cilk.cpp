#include "cilk.hpp"
#include "removeduplicates.hpp"

int main() {
  size_t n = taskparts::cmdline::parse_or_default_long("n", dflt_n);
  size_t r = taskparts::cmdline::parse_or_default_long("r", n);
  
  parlay::sequence<int> a;
  parlay::sequence<int> res;
  taskparts::benchmark_cilk([&] {
    res = dedup(a);
  }, [&] {
    a = dataGen::randIntRange<int>((size_t) 0,n,r);
  });
  return 0;
}
