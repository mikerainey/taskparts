#include <iostream>
#include <string>

#include <parlay/primitives.h>
#include <parlay/sequence.h>
#include <parlay/internal/get_time.h>

#include "BFS.h"
#include "helper/graph_utils.h"

// **************************************************************
// Driver
// **************************************************************
using vertex = int;
using nested_seq = parlay::sequence<parlay::sequence<vertex>>;
using graph = nested_seq;
using utils = graph_utils<vertex>;

int main(int argc, char* argv[]) {
  auto usage = "Usage: convert_graph <input filename> <output filename>";
  if (argc != 3) std::cout << usage << std::endl;
  else {
    graph G;
    G = utils::read_graph_from_file_pbbs(argv[1]);
    utils::print_graph_stats(G);
    utils::write_graph_to_file(G, argv[2]);
  }
}
