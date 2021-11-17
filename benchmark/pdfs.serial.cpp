#include <taskparts/benchmark.hpp>
#include "pdfs.hpp"

sequence<int> visited_serial;

auto DFS(vertexId source, const Graph& g) -> parlay::sequence<int> {
  auto nb_vertices = g.numVertices();
  parlay::sequence<vertexId> frontier(nb_vertices);
  vertexId frontier_size = 0;
  frontier[frontier_size++] = source;
  parlay::sequence<int> visited_serial(nb_vertices);
  visited_serial[source] = 1;
  while (frontier_size > 0) {
    vertexId vertex = frontier[--frontier_size];
    vertexId degree = g[vertex].degree;
    const vertexId* neighbors = g[vertex].Neighbors;
    for (vertexId edge = 0; edge < degree; edge++) {
      vertexId other = neighbors[edge];
      if (visited_serial[other])
        continue;
      visited_serial[other] = 1;
      frontier[frontier_size++] = other;
    }
  }
  return visited_serial;
}

auto benchmark_dflt_serial() {
  if (include_infile_load) {
    G = gen_input();
  }
  visited_serial = DFS(source, G);
}

auto benchmark_serial() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt_serial(); });
  } else {
    benchmark_dflt_serial();
  }
}

int main() {
  parlay::benchmark_taskparts([&] (auto sched) { // benchmark
    benchmark_serial();
  }, [&] (auto sched) { // setup
    load_input();
  }, [&] (auto sched) { // teardown
    size_t nb_visited = parlay::reduce(parlay::delayed_map(visited_serial, [&] (auto& p) -> size_t {
      return (size_t)p;}));
      cout << "total visited = " << nb_visited << endl;
  }, [&] (auto sched) { // reset
    visited_serial.clear();
  });
  return 0;
}
