#pragma once

#include "graph.hpp"

#ifndef PARLAY_SEQUENTIAL
#include <breadthFirstSearch/simpleBFS/BFS.C>
#else
#include <breadthFirstSearch/serialBFS/BFS.C>
#endif

Graph G;
sequence<vertexId> parents;
bool include_graph_gen;

auto benchmark() {
  if (include_graph_gen) {
    G = gen_input();
  }
  parents = BFS(source, G);  
}

auto load_input() {
  if (! include_graph_gen) {
    G = gen_input();
  }
}
