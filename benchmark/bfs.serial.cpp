#include <taskparts/benchmark.hpp>
#include "bfs.hpp"
#include <breadthFirstSearch/serialBFS/BFS.C>

int main() {
  Graph G;
  sequence<vertexId> parents;
  bool include_graph_gen = taskparts::cmdline::parse_or_default_bool("include_graph_gen", false);
  parlay::benchmark_taskparts([&] (auto sched) { // benchmark
    if (include_graph_gen) {
      G = gen_input();
    }
    parents = BFS(source, G);
  }, [&] (auto sched) { // setup
    if (! include_graph_gen) {
      G = gen_input();
    }
  }, [&] (auto sched) { // teardown
    size_t visited = parlay::reduce(parlay::delayed_map(parents, [&] (auto p) -> size_t {
      return (p == -1) ? 0 : 1;}));
    cout << "total visited = " << visited << endl;
  }, [&] (auto sched) { // reset
    parents.clear();
  });
  return 0;
}
