#include <stdio.h>
#include <assert.h>

#include "cilk.hpp"
#include "primes.hpp"

int main() {
  size_t n = taskparts::cmdline::parse_or_default_long("n", 10000000);
  taskparts::benchmark_cilk([&] {
    prime_sieve(n);    
  });
  return 0;
}
