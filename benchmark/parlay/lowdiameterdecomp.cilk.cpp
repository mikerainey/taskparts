#include "cilk.hpp"
#include <iostream>
#include <string>

#include <parlay/primitives.h>
#include <parlay/sequence.h>

#include "low_diameter_decomposition.h"
#include "helper/graph_utils.h"
#include "common.hpp"

using vertex = int;
using utils = graph_utils<vertex>;

long n = 0;
parlay::sequence<parlay::sequence<vertex>> G, GT;
vertex source = 1;

parlay::sequence<vertex> result;

auto gen_input() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  n = std::max((size_t)1, (size_t)taskparts::cmdline::parse_or_default_long("n", 5 * 1000 * 1000));
  source = taskparts::cmdline::parse_or_default_long("source", source);
  auto input = taskparts::cmdline::parse_or_default_string("input", "rmat");
  auto infile = input + ".adj";
  if (n == 0) {
    G = utils::read_symmetric_graph_from_file(infile.c_str());
    n = G.size();
  } else {
    G = utils::rmat_graph(n, 20*n);
    GT = utils::transpose(G);
  }
#ifndef NDEBUG
  utils::print_graph_stats(G);
#endif
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  result = LDD<vertex>(.5, G, GT);
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}

int main() {
  taskparts::benchmark_cilk([&] {
    benchmark();
  }, [&] { // setup
    gen_input();
  }, [&] { // teardown
    auto cluser_ids = parlay::remove_duplicates(result);
    std::cout << "nb_clusters " << cluser_ids.size() << std::endl;
  }, [&] { // reset
    result.clear();
  });
  return 0;
}
