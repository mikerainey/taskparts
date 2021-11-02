#include "cilk.hpp"
#include "bfs.hpp"

int main() {
  Graph G;
  sequence<vertexId> parents;
  bool include_graph_gen = taskparts::cmdline::parse_or_default_bool("include_graph_gen", false);
  taskparts::benchmark_cilk([&] { // benchmark
    if (include_graph_gen) {
      G = gen_input();
    }
    parents = BFS(source, G);
  }, [&] { // setup
    if (! include_graph_gen) {
      G = gen_input();
    }
  }, [&] { // teardown
    size_t visited = parlay::reduce(parlay::delayed_map(parents, [&] (auto p) -> size_t {
      return (p == -1) ? 0 : 1;}));
    cout << "total visited = " << visited << endl;
  }, [&] { // reset
    parents.clear();
  });
  return 0;
}
