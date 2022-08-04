#include "graph.hpp"
#include "bellman_ford.h"

using utils = graph_utils<vertex>;
using wtype = float;
utils::weighted_graph<wtype> WG;
parlay::sequence<wtype> result;

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  result = *bellman_ford_lazy<wtype>(1, WG, WG);
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
    WG = utils::add_weights<wtype>(G, 1.0, 20.0);
  }, [&] (auto sched) { // teardown
    std::cout << "result " << result.size() << std::endl;
  }, [&] (auto sched) { // reset

  });
  return 0;
}
