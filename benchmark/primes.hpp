#pragma once

#include <parlay/delayed_sequence.h>
#include <parlay/monoid.h>
#include <parlay/primitives.h>
#include <parlay/parallel.h>

// ------------------------- Prime Sieve -----------------------------

// Recursively finds primes less than sqrt(n), then sieves out
// all their multiples, returning the primes less than n.
// Work is O(n log log n), Span is O(log n).
template <class Int>
parlay::sequence<Int> prime_sieve(Int n) {
  if (n < 2) return parlay::sequence<Int>();
  else {
    Int sqrt = std::sqrt(n);
    auto primes_sqrt = prime_sieve(sqrt);
    parlay::sequence<bool> flags(n+1, true);  // flags to mark the primes
    flags[0] = flags[1] = false;              // 0 and 1 are not prime
    parlay::parallel_for(0, primes_sqrt.size(), [&] (size_t i) {
      Int prime = primes_sqrt[i];
      parlay::parallel_for(2, n/prime + 1, [&] (size_t j) {
	      flags[prime * j] = false;
	    }, 1000);
    }, 1);
    return parlay::pack_index<Int>(flags);    // indices of the primes
  }
}
