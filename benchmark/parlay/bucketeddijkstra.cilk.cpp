#include "cilk.hpp"
#include "graph.hpp"
#include "bucketed_dijkstra.h"

using wtype = int;
utils::weighted_graph<wtype> GW;
nested_seq result;

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  result = bucketed_dijkstra(source, GW);
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
    gen_input();
    GW = utils::add_weights<int>(G,1,20);
  }, [&] { // teardown
    std::cout << "result " << result.size() << std::endl;
  }, [&] { // reset
    result.clear();
  });
  return 0;
}
