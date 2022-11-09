#include <iostream>
#include <string>
#include <random>
#include <tuple>

#include <parlay/primitives.h>
#include <parlay/random.h>
#include <parlay/sequence.h>
#include <parlay/internal/get_time.h>

#include "huffman_tree.h"
#include "common.hpp"

std::tuple<parlay::sequence<node*>,node*> result;
parlay::sequence<float> probs;
long n;

auto gen_input() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  n = std::max((size_t)1, (size_t)taskparts::cmdline::parse_or_default_long("n", 6 * 1000 * 1000));
  parlay::random_generator gen;
  std::uniform_real_distribution<float> dis(1.0, static_cast<float>(n));
  
  // generate unnormalized probabilities
  probs = parlay::tabulate(n, [&] (long i) -> float {
    auto r = gen[i];
    return 1.0/dis(r);
  });
  
  // normalize them
  float total = parlay::reduce(probs);
  probs = parlay::map(probs, [&] (float p) {return p/total;});
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  result = huffman_tree(probs);
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}

int main() {
  parlay::benchmark_taskparts([&] (auto sched) { // benchmark
    benchmark();
  }, [&] (auto sched) { // setup
    gen_input();
  }, [&] (auto sched) { // teardown
    delete_tree(std::get<1>(result));
  }, [&] (auto sched) { // reset
    delete_tree(std::get<1>(result));
  });
  return 0;
}
