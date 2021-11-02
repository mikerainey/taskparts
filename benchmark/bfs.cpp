#include <taskparts/benchmark.hpp>
#include "bfs.hpp"

int main() {
  Graph G;
  sequence<vertexId> parents;
  parlay::benchmark_taskparts([&] (auto sched) { // benchmark
    parents = BFS(source, G);
  }, [&] (auto sched) { // setup
    G = gen_input();
  }, [&] (auto sched) { // teardown
    size_t visited = parlay::reduce(parlay::delayed_map(parents, [&] (auto p) -> size_t {
      return (p == -1) ? 0 : 1;}));
    cout << "total visited = " << visited << endl;
  }, [&] (auto sched) { // reset
    parents.clear();
  });
  return 0;
}
