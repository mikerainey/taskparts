#include "cilk.hpp"
#include "bfs.hpp"

int main() {
  Graph G;
  sequence<vertexId> parents;
  taskparts::benchmark_cilk([&] { // benchmark
    parents = BFS(source, G);
  }, [&] { // setup
    G = gen_input();
  }, [&] { // teardown
    size_t visited = parlay::reduce(parlay::delayed_map(parents, [&] (auto p) -> size_t {
      return (p == -1) ? 0 : 1;}));
    cout << "total visited = " << visited << endl;
  }, [&] { // reset
    parents.clear();
  });
  return 0;
}
