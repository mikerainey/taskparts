#include <iostream>
#include <string>

#include <parlay/primitives.h>
#include <parlay/sequence.h>
#include <parlay/internal/get_time.h>

#include "push_relabel_max_flow.h"
#include "helper/graph_utils.h"
#include "common.hpp"

using utils = graph_utils<vertex_id>;
long n = 0;
utils::graph G;
utils::weighted_graph<weight> GW;
int result = 0;
using edge = std::pair<vertex_id,weight>;
vertex_id source = 1;

auto gen_input() {
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
    G = utils::rmat_symmetric_graph(n, 20*n);
  }
#ifndef NDEBUG
  utils::print_graph_stats(G);
#endif
  GW = utils::add_weights<weight>(G,1,1);
  edges s_e = parlay::tabulate(n/4, [&] (long i) {return edge(i, n);});
  edges t_e = parlay::tabulate(n/4, [&] (long i) {return edge(n - n/4 + i, n);});
  GW.push_back(s_e);
  GW.push_back(t_e);
  parlay::parallel_for(0, n/4, [&] (long i) {
    GW[i].push_back(edge(n, n));
    GW[n - n/4 + i].push_back(edge(n+1, n));
  });
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  result = maximum_flow(GW, n, n+1);
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
    std::cout << "max_flow " << result << std::endl;
  }, [&] (auto sched) { // reset
    result = 0;
  });
  return 0;
}
