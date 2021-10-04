#include <taskparts/benchmark.hpp>
#include "primes.hpp"

int main() {
  size_t n = taskparts::cmdline::parse_or_default_long("n", 10000000);
  parlay::benchmark_taskparts([&] (auto sched) {
    prime_sieve(n);    
  });
  return 0;
}
