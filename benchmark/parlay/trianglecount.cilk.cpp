#include "cilk.hpp"
#include "common.hpp"
#include "triangle_count.h"
#include "helper/graph_utils.h"

using vertex = int;
using utils = graph_utils<vertex>;
long n = 0;
graph<vertex> G;
vertex source = 1;
long count;

auto gen_input2() {
  std::string infile_path = "";
  if (const auto env_p = std::getenv("TASKPARTS_BENCHMARK_INFILE_PATH")) {
    infile_path = std::string(env_p);
  }
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  n = std::max((size_t)1, (size_t)taskparts::cmdline::parse_or_default_long("n", 1 * 1000 * 1000));
  source = taskparts::cmdline::parse_or_default_long("source", source);
  auto input = taskparts::cmdline::parse_or_default_string("input", "europe");
  if (input != "gen-rmat") {
    auto infile = infile_path + "/" + input + ".adj";
    G = utils::symmetrize(utils::read_graph_from_file_pbbs(infile.c_str()));
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
  count = triangle_count(G);
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
    std::cout << "nb_triangles " << count << std::endl;
  }, [&] { // reset

  });
  return 0;
}
