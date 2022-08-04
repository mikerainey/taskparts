#include "graph.hpp"
#include "BFS.h"

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  result = BFS(source, G);
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
    std::cout << "result " << result.size() << std::endl;
  }, [&] (auto sched) { // reset

  });
  return 0;
}
