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
vertex source = 0;

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
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  n = std::max((size_t)1, (size_t)taskparts::cmdline::parse_or_default_long("n", 1 * 1000 * 1000));
  source = taskparts::cmdline::parse_or_default_long("source", source);
  auto input = taskparts::cmdline::parse_or_default_string("input", "rmat");
  auto infile = input + ".adj";
  if (n == 0) {
    G = utils::read_symmetric_graph_from_file(infile.c_str());
    n = G.size();
  } else {
    G = utils::rmat_graph(n, 20*n);
  }
#ifndef NDEBUG
  utils::print_graph_stats(G);
#endif
}

