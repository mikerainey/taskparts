#pragma once

#include "graph.hpp"

#ifndef PARLAY_SEQUENTIAL
#include <breadthFirstSearch/simpleBFS/BFS.C>
#else
#include <breadthFirstSearch/serialBFS/BFS.C>
#endif

Graph G;
sequence<vertexId> parents;

auto benchmark_dflt() {
  if (include_infile_load) {
    G = gen_input();
  }
  parents = BFS(source, G);  
}

auto load_input() {
  if (! include_infile_load) {
    G = gen_input();
  }
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}
