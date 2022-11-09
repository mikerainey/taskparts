#include <string>
#include <utility>

#include <parlay/primitives.h>
#include <parlay/sequence.h>
#include <parlay/internal/get_time.h>

#include "kruskal.h"
#include "graph.hpp"

parlay::sequence<long> result;
utils::weighted_edges<float> WE;

auto gen_input2() {
  std::string infile_path = "";
  if (const auto env_p = std::getenv("TASKPARTS_BENCHMARK_INFILE_PATH")) {
    infile_path = std::string(env_p);
  }
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  n = std::max((size_t)1, (size_t)taskparts::cmdline::parse_or_default_long("n", 4 * 1000 * 1000));
  source = taskparts::cmdline::parse_or_default_long("source", 0);
  auto input = taskparts::cmdline::parse_or_default_string("input", "orkut");
  if (input != "gen-rmat") {
    auto infile = infile_path + "/" + input + ".adj";
    G = utils::read_graph_from_file_pbbs(infile.c_str());
    n = G.size();
  } else {
    E = utils::rmat_edges(n, 20*n);
    n = utils::num_vertices(E);
  }
  E = utils::to_edges(G);
#ifndef NDEBUG
  utils::print_graph_stats(E,n);
#endif
  WE = utils::add_weights<float>(E);

}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input2();
  }
  result = min_spanning_forest(WE, n);
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
    std::cout << "forest_sz " << result.size() << std::endl;
  }, [&] (auto sched) { // reset
    result.clear();
  });
  return 0;
}
