#include <taskparts/benchmark.hpp>
#include <parlay/delayed_sequence.h>
#include <parlay/monoid.h>
#include <parlay/primitives.h>
#include <parlay/parallel.h>

// ---------------------------- Integration ----------------------------

template <typename F>
double integrate(size_t num_samples, double start, double end, F f) {
   double delta = (end-start)/num_samples;
   auto samples = parlay::delayed_seq<double>(num_samples, [&] (size_t i) {
        return f(start + delta/2 + i * delta); });
   return delta * parlay::reduce(samples, parlay::addm<double>());
}

int main() {
  size_t n = taskparts::cmdline::parse_or_default_long("n", 100000000);
  auto f = [&] (double q) -> double {
    return pow(q, 2);
  };
  double start = static_cast<double>(0);
  double end = static_cast<double>(1000);
  parlay::benchmark_taskparts([&] (auto sched) {
    integrate(n, start, end, f);
  });
  return 0;
}
