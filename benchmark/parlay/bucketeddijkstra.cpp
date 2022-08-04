#include "graph.hpp"
#include "bucketed_dijkstra.h"

using wtype = int;
utils::weighted_graph<wtype> GW;
nested_seq result;

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  result = bucketed_dijkstra(1, GW);
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
    GW = utils::add_weights<int>(G,1,20);
  }, [&] (auto sched) { // teardown
    std::cout << "result " << result.size() << std::endl;
  }, [&] (auto sched) { // reset

  });
  return 0;
}
