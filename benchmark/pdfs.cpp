#include <taskparts/benchmark.hpp>
#include "pdfs.hpp"
#include "frontierseg.hpp"

class graph_alias {
public:
  using vtxid = vertexId;
  const vertexId* degrees;
  const vertexId* edges;
  const edgeId* offsets;
  graph_alias(vertexId* degrees=nullptr, vertexId* edges=nullptr, edgeId* offsets=nullptr)
    : degrees(degrees), edges(edges), offsets(offsets) { }
  graph_alias(const Graph& G)
    : degrees(G.degrees.data()), edges(G.edges.data()), offsets(G.offsets.data()) {
    assert(G.edges.size() > 0);
  }
  vertexId get_degree(vertexId i) {
    return degrees[i];
  }
  const vertexId* get_neighbors(vertexId i) {
    return edges + offsets[i];
  }
};

size_t poll_cutoff = 128;

bool try_to_mark(parlay::sequence<std::atomic<int>>& visited, vertexId target) {
  int orig = 0;
  if (! visited[target].compare_exchange_strong(orig, 1))
    return false;
  return true;
}

auto PDFS(vertexId source, const Graph& g) -> parlay::sequence<std::atomic<int>> {
  pasl::graph::frontiersegbag<graph_alias> frontier;
  frontier.set_graph(graph_alias(g));
  frontier.push_vertex_back(source);
  auto nb_vertices = g.numVertices();
  parlay::sequence<std::atomic<int>> visited(nb_vertices);
  visited[source].store(1);
  size_t nb_since_last_split;
  while (frontier.nb_outedges() > 0) {
    nb_since_last_split +=    
      frontier.for_at_most_nb_outedges(poll_cutoff, [&] (vertexId other_vertex) {
	if (try_to_mark(visited, other_vertex))
	  frontier.push_vertex_back(other_vertex);
      });
  }
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
