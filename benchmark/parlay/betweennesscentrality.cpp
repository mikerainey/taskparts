#include "graph.hpp"
#include "betweenness_centrality.h"

parlay::sequence<float> result;

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  result = BC_single_source(source, G, GT);
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
    if (n == 0) {
      GT = G;
      n = G.size();
    } else {
      GT = utils::transpose(G);
    }
  }, [&] (auto sched) { // teardown
#ifndef NDEBUG
    long max_centrality = parlay::reduce(result, parlay::maximum<float>());
    std::cout << "max betweenness centrality " << max_centrality << std::endl;
#endif
  }, [&] (auto sched) { // reset
    result.clear();
  });
  return 0;
}
