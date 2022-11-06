#include "cilk.hpp"
#include "graph.hpp"
#include "graph_color.h"

parlay::sequence<int> colors;

auto gen_input2() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  n = std::max((size_t)1, (size_t)taskparts::cmdline::parse_or_default_long("n", 4 * 1000 * 1000));
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

bool check(const graph& G, const parlay::sequence<int> colors) {
  auto is_good = [&] (long u) {
    return all_of(G[u], [&] (auto& v) {return colors[u] != colors[v];});};
  return count(parlay::tabulate(G.size(), is_good), true) == G.size();
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input2();
  }
  colors = graph_coloring(G);
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
    int max_color = parlay::reduce(colors, parlay::maximum<int>());
    if (!check(G, colors)) std::cout << "bad coloring" << std::endl;
    else std::cout << "nb_colors " << max_color + 1  << std::endl;
  }, [&] { // reset
    colors.clear();
  });
  return 0;
}
