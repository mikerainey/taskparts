#include <iostream>
#include <string>
#include <utility>

#include <parlay/primitives.h>
#include <parlay/sequence.h>

#include "pagerank.h"
#include "graph.hpp"

using element = std::pair<vertex,float>;
using row = parlay::sequence<element>;
using sparse_matrix = parlay::sequence<row>;
using vector = parlay::sequence<double>;

vector v;
sparse_matrix M;

auto gen_input2() {
  gen_input();
  M = utils::to_normalized_matrix(G);
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input2();
  }
  v = pagerank(M, 10);
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
    gen_input2();
  }, [&] (auto sched) { // teardown
    double maxv = *parlay::max_element(v);
    std::cout << "max_rank " << maxv * n << std::endl;
  }, [&] (auto sched) { // reset
    v.clear();
  });
  return 0;
}
