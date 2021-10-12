#include "cilk.hpp"
#include "bfs.hpp"

int main() {
  auto infile = taskparts::cmdline::parse_or_default_string("infile", "randlocal.adj");
  Graph G;
  sequence<vertexId> parents;
  size_t source = 0;
  taskparts::benchmark_cilk([&] { // benchmark
    parents = BFS(source, G);
  }, [&] { // setup
    G = readGraphFromFile<vertexId,edgeId>((char*)infile.c_str());
    G.addDegrees();
  }, [&] { // teardown
    size_t visited = parlay::reduce(parlay::delayed_map(parents, [&] (auto p) -> size_t {
      return (p == -1) ? 0 : 1;}));
    cout << "total visited = " << visited << endl;
  }, [&] { // reset
    parents.clear();
  });
  return 0;
}
