#include "graph.hpp"
#include "BFS_ligra.h"

nested_seq result;

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  result = BFS(source, G, GT);
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
    std::cout << "result " << result.size() << std::endl;
  }, [&] (auto sched) { // reset
    result.clear();
  });
  return 0;
}
