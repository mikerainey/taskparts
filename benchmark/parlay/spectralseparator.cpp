#include "common.hpp"
#include "spectral_separator.h"
#include "helper/graph_utils.h"

using utils = graph_utils<vertex>;
long n = 0;
graph G;
parlay::sequence<bool> partition;

auto gen_input2() {
  std::string infile_path = "";
  if (const auto env_p = std::getenv("TASKPARTS_BENCHMARK_INFILE_PATH")) {
    infile_path = std::string(env_p);
  }
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  n = std::max((size_t)1, (size_t)taskparts::cmdline::parse_or_default_long("n", 4 * 1000 * 1000));
  auto input = taskparts::cmdline::parse_or_default_string("input", "europe");
  if (input != "gen-rmat") {
    auto infile = infile_path + "/" + input + ".adj";
    G = utils::read_graph_from_file_pbbs(infile.c_str());
    n = G.size();
  } else {
  G = utils::grid_graph(n);
  }
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
  parlay::benchmark_taskparts([&] (auto sched) { // benchmark
    benchmark();
  }, [&] (auto sched) { // setup
    gen_input2();
  }, [&] (auto sched) { // teardown
    auto E = utils::to_edges(G);
    long num_in_cut = parlay::count(parlay::map(E, [&] (auto e) {
      return partition[e.first] != partition[e.second];}), true);
    std::cout << "nb_edges_across_cut " << num_in_cut << std::endl;
  }, [&] (auto sched) { // reset
    partition.clear();
  });
  return 0;
}
