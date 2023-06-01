#include <iostream>
#include <string>
#include <random>

#include <parlay/io.h>
#include <parlay/primitives.h>
#include <parlay/random.h>
#include <parlay/sequence.h>

#include "mergesort.h"
#include "benchmark.hpp"

// **************************************************************
// Driver
// **************************************************************
int main(int argc, char* argv[]) {
  auto usage = "Usage: mergesort <n>";
  if (argc != 2) std::cout << usage << std::endl;
  else {
    long n;
    try { n = std::stol(argv[1]); }
    catch (...) { std::cout << usage << std::endl; return 1; }
    parlay::random_generator gen;
    std::uniform_int_distribution<long> dis(0, n-1);

    // generate random long values
    auto data = parlay::tabulate(n, [&] (long i) {
      auto r = gen[i];
      return dis(r);});

    parlay::internal::timer t("Time");
    parlay::sequence<long> result = data;
    taskparts::benchmark([&] {
      merge_sort(result);
    }, [] {}, [] {}, [&] {
      result = data;
    });

    auto first_ten = result.head(10);

    std::cout << "first 10 elements: " << parlay::to_chars(first_ten) << std::endl;
  }
}
