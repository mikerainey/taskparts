#include <taskparts/benchmark.hpp>
#include <parlay/delayed_sequence.h>
#include <parlay/monoid.h>
#include <parlay/primitives.h>
#include <parlay/parallel.h>
#include <testData/sequenceData/sequenceData.h>
#include <removeDuplicates/parlayhash/dedup.h>

int main() {
  size_t n = taskparts::cmdline::parse_or_default_long("n", 10000000);
  size_t r = taskparts::cmdline::parse_or_default_long("r", n);
  
  parlay::sequence<int> a;
  parlay::sequence<int> res;
  parlay::benchmark_taskparts([&] (auto sched) {
    res = dedup(a);
  }, [&] (auto sched) {
    a = dataGen::randIntRange<int>((size_t) 0,n,r);
  });
  return 0;
}
