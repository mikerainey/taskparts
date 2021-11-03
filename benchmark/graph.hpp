#pragma once

#include <parlay/delayed_sequence.h>
#include <parlay/monoid.h>
#include <parlay/primitives.h>
#include <parlay/parallel.h>

#include <common/graph.h>
#include <common/IO.h>
#include <common/graphIO.h>
#include <common/sequenceIO.h>
#include <testData/graphData/alternatingGraph.h>
#include <testData/graphData/rMat.h>

#include <taskparts/cmdline.hpp>
#include <breadthFirstSearch/bench/BFS.h>

using namespace std;
using namespace benchIO;

size_t source = 0;

auto gen_input() {
  Graph G;
  source = taskparts::cmdline::parse_or_default_long("source", source);
  taskparts::cmdline::dispatcher d;
  d.add("alternating", [&] {
    size_t n =
      taskparts::cmdline::parse_or_default_long("n", 3);
    // for ~85% utilization on 15 cores:
    //   -nb_vertices_per_phase 700000 -nb_in_chains_between_phases 800000
    size_t nb_vertices_per_phase =
      taskparts::cmdline::parse_or_default_long("nb_vertices_per_phase", 400000);
    size_t nb_per_phase_at_max_arity =
      taskparts::cmdline::parse_or_default_long("nb_per_phase_at_max_arity", 3);
    size_t arity_of_vertices_not_at_max_arity =
      taskparts::cmdline::parse_or_default_long("arity_of_vertices_not_at_max_arity", 1);
    size_t nb_in_chains_between_phases =
      taskparts::cmdline::parse_or_default_long("nb_in_chains_between_phases", 3000000);
    size_t nb_alternations =
      taskparts::cmdline::parse_or_default_long("nb_alternations", 3);
    auto EA = alternating<vertexId>(n, nb_vertices_per_phase, nb_per_phase_at_max_arity,
				    arity_of_vertices_not_at_max_arity,
				    nb_in_chains_between_phases,
				    nb_alternations);
    G = graphFromEdges<vertexId,edgeId>(EA, true);
  });
  d.add("rMat", [&] {
    size_t n = taskparts::cmdline::parse_or_default_long("n", 5000000);
    double a = 0.5;
    double b = 0.1;
    double c = b;
    size_t m = 10*n;
    size_t seed = 1;
    auto EA = edgeRmat<vertexId>(n, m, seed, a, b, c);
    G = graphFromEdges<vertexId,edgeId>(EA, true);    
  });
  d.add("from_file", [&] {
    auto infile = taskparts::cmdline::parse_or_default_string("infile", "randlocal.adj");
    G = readGraphFromFile<vertexId,edgeId>((char*)infile.c_str());
  });
  d.dispatch_or_default("input", "rMat");
  G.addDegrees();
  return G;
}
