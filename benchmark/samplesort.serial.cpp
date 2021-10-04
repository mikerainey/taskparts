#include <taskparts/benchmark.hpp>
#include <parlay/delayed_sequence.h>
#include <parlay/monoid.h>
#include <parlay/primitives.h>
#include <parlay/parallel.h>
#include <testData/sequenceData/sequenceData.h>
#include <comparisonSort/serialSort/sort.h>

size_t dflt_n = 10000000;

int main() {
  size_t n = taskparts::cmdline::parse_or_default_long("n", dflt_n);
  size_t r = taskparts::cmdline::parse_or_default_long("r", n);

  parlay::sequence<long> a;
  parlay::benchmark_taskparts([&] (auto sched) {
    compSort(a, [] (long x, long y) { return x < y; });
  }, [&] (auto sched) {
    a = dataGen::randIntRange<long>((size_t) 0,n,r);
  });
  return 0;
}
