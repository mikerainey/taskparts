#pragma once

#include "common.hpp"
#include <common/graph.h>
#include <common/IO.h>
#include <common/graphIO.h>
#include <common/sequenceIO.h>
#include <testData/graphData/alternatingGraph.h>
#include <testData/graphData/rMat.h>
#include <benchmarks/breadthFirstSearch/bench/BFS.h>

using namespace std;
using namespace benchIO;

size_t source = 0;

auto gen_alternating() {
  Graph G;
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
  return G;
}

auto gen_input() {
  std::string infile_path = "";
  if (const auto env_p = std::getenv("TASKPARTS_BENCHMARK_INFILE_PATH")) {
    infile_path = std::string(env_p);
  }
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  Graph G;
  source = taskparts::cmdline::parse_or_default_long("source", source);
  auto input = taskparts::cmdline::parse_or_default_string("input", "rmat");
  auto infile = infile_path + "/" + input + ".adj";
  G = readGraphFromFile<vertexId,edgeId>((char*)infile.c_str());
  /*
  if (include_infile_load) {
    auto infile = input + ".adj";
    G = readGraphFromFile<vertexId,edgeId>((char*)infile.c_str());
  } else {
    if (input == "alternating") {
      gen_alternating();
    } else { // input == "rmat"
      size_t n = taskparts::cmdline::parse_or_default_long("n", 5000000);
      double a = 0.5;
      double b = 0.1;
      double c = b;
      size_t m = 10*n;
      size_t seed = 1;
      auto EA = edgeRmat<vertexId>(n, m, seed, a, b, c);
      G = graphFromEdges<vertexId,edgeId>(EA, true);    
    } 
    } */
  G.addDegrees();
  return G;
}
