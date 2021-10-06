#include <taskparts/benchmark.hpp>
#include "pdfs.hpp"
#include "frontierseg.hpp"

auto PDFS(vertexId source, const Graph& g) -> parlay::sequence<std::atomic<int>> {
  pasl::graph::frontiersegbag<Graph> fs;
  auto nb_vertices = g.numVertices();
  parlay::sequence<std::atomic<int>> visited(nb_vertices);
  parlay::parallel_for(0, nb_vertices, [&] (size_t i) {
    visited[i].store(0);
  });
  return visited;
}

int main() {
  auto fname = taskparts::cmdline::parse_or_default_string("fname", "randlocal.adj");
  Graph G;
  sequence<std::atomic<int>> visited;
  size_t source = 0;
  parlay::benchmark_taskparts([&] (auto sched) { // benchmark
    visited = PDFS(source, G);
  }, [&] (auto sched) { // setup
    G = readGraphFromFile<vertexId,edgeId>((char*)fname.c_str());
    G.addDegrees();
  }, [&] (auto sched) { // teardown
    size_t nb_visited = parlay::reduce(parlay::delayed_map(visited, [&] (auto& p) -> size_t {
      return (size_t)p.load();}));
      cout << "total visited = " << nb_visited << endl;
  }, [&] (auto sched) { // reset
    visited.clear();
  });
  return 0;
}
