#pragma once

#include <iostream>
#include <string>
#include "common.hpp"
#include "helper/graph_utils.h"

// **************************************************************
// Driver
// **************************************************************
using vertex = int;
using nested_seq = parlay::sequence<parlay::sequence<vertex>>;
using graph = nested_seq;
using utils = graph_utils<vertex>;

long n = 0;
graph G, GT;
utils::edges E;
vertex source = 1;

/*
auto gen_alternating() {
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
*/

auto gen_input() {
  std::string infile_path = "";
  if (const auto env_p = std::getenv("TASKPARTS_BENCHMARK_INFILE_PATH")) {
    infile_path = std::string(env_p);
  }
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  n = std::max((size_t)1, (size_t)taskparts::cmdline::parse_or_default_long("n", 4 * 1000 * 1000));
  source = taskparts::cmdline::parse_or_default_long("source", source);
  auto input = taskparts::cmdline::parse_or_default_string("input", "rmat");
  if (input == "rmat") {
    G = utils::rmat_graph(n, 20*n);
  } else {
    auto infile = infile_path + "/" + input + ".adj";
    G = utils::read_graph_from_file_pbbs(infile.c_str());
    n = G.size();
  }
#ifndef NDEBUG
  utils::print_graph_stats(G);
#endif
}

