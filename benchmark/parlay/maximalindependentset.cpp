#include "graph.hpp"
#include "maximal_independent_set.h"

parlay::sequence<bool> in_set;

auto gen_input2() {
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
    G = utils::rmat_symmetric_graph(n, 20*n);
  }
#ifndef NDEBUG
  utils::print_graph_stats(G);
#endif
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input2();
  }
  in_set = MIS(G);
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
#ifndef NDEBUG
    int num_in_set = parlay::reduce(parlay::map(in_set,[] (bool a) {return a ? 1 : 0;}));
    std::cout << "number in set: " << num_in_set << std::endl;
#endif
  }, [&] (auto sched) { // reset

  });
  return 0;
}
