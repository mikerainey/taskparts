#pragma once

#define TASKPARTS_TPALRTS_MINIMAL // disables heartbeat interrupts
#include <taskparts/benchmark.hpp>
#include "pbbsgraph.hpp"
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

bool try_to_mark(parlay::sequence<std::atomic<int>>& visited, vertexId target) {
  int orig = 0;
  if (! visited[target].compare_exchange_strong(orig, 1)) {
    return false;
  }
  return true;
}

using frontier_type = pasl::graph::frontiersegbag<graph_alias>;

Graph G;

auto load_input() {
  if (! include_infile_load) {
    G = gen_input();
  }
}

#ifndef TASKPARTS_SERIAL_ELISION
namespace taskparts {
template <typename F1, typename F2, typename Fj, typename Scheduler>
auto tpalrts_promote_via_nativefj(const F1& f1, const F2& f2, const Fj& fj, Scheduler sched) {
  using fiber_type = nativefj_fiber<Scheduler>;
  nativefj_from_lambda<decltype(f1), Scheduler> fb1(f1);
  nativefj_from_lambda<decltype(f2), Scheduler> fb2(f2);
  nativefj_from_lambda<decltype(fj), Scheduler> fbj(fj);
  auto cfb = fiber_type::current_fiber.mine();
  cfb->status = fiber_status_pause;
  fiber<Scheduler>::add_edge(&fb1, &fbj);
  fiber<Scheduler>::add_edge(&fb2, &fbj);
  fiber<Scheduler>::add_edge(&fbj, cfb);
  fb1.release();
  fb2.release();
  fbj.release();
  if (context::capture<fiber_type*>(context::addr(cfb->ctx))) {
    return;
  }
  fb1.stack = fiber_type::notownstackptr;
  fb1.swap_with_scheduler();
  fb1.run();
  auto f = Scheduler::template take<nativefj_fiber>();
  if (f == nullptr) {
    cfb->status = fiber_status_finish;
    cfb->exit_to_scheduler();
    return; // unreachable
  }
  fb2.stack = fiber_type::notownstackptr;
  fb2.swap_with_scheduler();
  fb2.run();
  cfb->status = fiber_status_finish;
  cfb->swap_with_scheduler();
}
} // end namespace

void loop(const Graph& g, frontier_type& _frontier, parlay::sequence<std::atomic<int>>& visited) {
  size_t poll_cutoff = 10000;
  size_t split_cutoff = 10000;
  size_t cutoff = 10000;
  frontier_type frontier;
  frontier.swap(_frontier);
  size_t nb_since_last_split = 0;
  size_t f = frontier.nb_outedges();
  while (f > 0) {
    if ((f > split_cutoff)						  //
	|| ((nb_since_last_split > split_cutoff) && (f > 1))) {		  //
      TASKPARTS_LOG_PPT(taskparts::bench_logging, 0);
      frontier_type frontier1, frontier2;
      frontier1.set_graph(graph_alias(g));
      frontier2.set_graph(graph_alias(g));
      vertexId m = vertexId(frontier.nb_outedges() / 2); // smaller half given away
      frontier.split(m, frontier1);
      frontier.swap(frontier2);
      tpalrts_promote_via_nativefj([&] {
	loop(g, frontier1, visited);
      }, [&] {
	loop(g, frontier2, visited);
      }, [&] { }, taskparts::bench_scheduler());
      TASKPARTS_LOG_PPT(taskparts::bench_logging, 0);
    }
    nb_since_last_split +=    
      frontier.for_at_most_nb_outedges(poll_cutoff, [&] (vertexId other_vertex) {
	if (try_to_mark(visited, other_vertex)) {
	  frontier.push_vertex_back(other_vertex);
	}
      });
    f = frontier.nb_outedges();
  }
}

auto PDFS(vertexId source, const Graph& g) -> parlay::sequence<std::atomic<int>> {
  frontier_type frontier0;
  frontier0.set_graph(graph_alias(g));
  frontier0.push_vertex_back(source);
  parlay::sequence<std::atomic<int>> visited(g.numVertices());
  TASKPARTS_LOG_PPT(taskparts::bench_logging, 0);
  visited[source].store(1);
  loop(g, frontier0, visited);
  TASKPARTS_LOG_PPT(taskparts::bench_logging, 0);
  return visited;
}

sequence<std::atomic<int>> visited;

auto benchmark_dflt() {
  if (include_infile_load) {
    G = gen_input();
  }
  visited = PDFS(source, G);
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}
 #endif
