#include "cilk.hpp"
#include "common.hpp"
#include "spectral_separator.h"
#include "helper/graph_utils.h"

using utils = graph_utils<vertex>;
long n = 0;
graph G;
parlay::sequence<bool> partition;

auto gen_input2() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  n = std::max((size_t)1, (size_t)taskparts::cmdline::parse_or_default_long("n", 50 * 1000));
  G = utils::grid_graph(n);
#ifndef NDEBUG
  utils::print_graph_stats(G);
#endif
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input2();
  }
  partition = partition_graph(G);
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
    gen_input2();
  }, [&] { // teardown
    auto E = utils::to_edges(G);
    long num_in_cut = parlay::count(parlay::map(E, [&] (auto e) {
      return partition[e.first] != partition[e.second];}), true);
    std::cout << "nb_edges_across_cut " << num_in_cut << std::endl;
  }, [&] { // reset
    partition.clear();
  });
  return 0;
}
