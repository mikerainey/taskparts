#include <taskparts/benchmark.hpp>
#include "pdfs.hpp"
#include "frontierseg.hpp"

auto DFS(vertexId source, const Graph& g) -> parlay::sequence<int> {
  auto nb_vertices = g.numVertices();
  parlay::sequence<vertexId> frontier(nb_vertices);
  vertexId frontier_size = 0;
  frontier[frontier_size++] = source;
  parlay::sequence<int> visited(nb_vertices);
  visited[source] = 1;
  while (frontier_size > 0) {
    vertexId vertex = frontier[--frontier_size];
    vertexId degree = g[vertex].degree;
    const vertexId* neighbors = g[vertex].Neighbors;
    for (vertexId edge = 0; edge < degree; edge++) {
      vertexId other = neighbors[edge];
      if (visited[other])
        continue;
      visited[other] = 1;
      frontier[frontier_size++] = other;
    }
  }
  return visited;
}

int main() {
  Graph G;
  sequence<int> visited;
  bool include_graph_gen = taskparts::cmdline::parse_or_default_bool("include_graph_gen", false);
  parlay::benchmark_taskparts([&] (auto sched) { // benchmark
    if (include_graph_gen) {
      G = gen_input();
    }
    visited = DFS(source, G);
  }, [&] (auto sched) { // setup
    if (! include_graph_gen) {
      G = gen_input();
    }
  }, [&] (auto sched) { // teardown
    size_t nb_visited = parlay::reduce(visited);
      cout << "total visited = " << nb_visited << endl;
  }, [&] (auto sched) { // reset
    visited.clear();
  });
  return 0;
}
