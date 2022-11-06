#include "common.hpp"
#include "set_cover.h"
#include "helper/graph_utils.h"

#include "cilk.hpp"

using idx = int;  // the index of an element or set
using sets = parlay::sequence<parlay::sequence<idx>>;
using set_ids = parlay::sequence<idx>;
using utils = graph_utils<idx>;

float epsilon = .05;
long n = 0;
sets S;
set_ids r;

// Check answer is correct (i.e. covers all elements that could be)
bool check(const set_ids& SI, const sets& S, long num_elements) {
  parlay::sequence<bool> a(num_elements, true);

  // set all that could be covered by original sets to false
  parlay::parallel_for(0, S.size(), [&] (long i) {
    for (idx j : S[i]) a[j] = false;});

  // set all that are covered by SI back to true
  parlay::parallel_for(0, SI.size(), [&] (long i) {
    for (idx j : S[SI[i]]) a[j] = true;});

  // check that all are covered
  return (parlay::count(a, true) == num_elements);
}

auto gen_input2() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  n = std::max((size_t)1, (size_t)taskparts::cmdline::parse_or_default_long("n", 500 * 1000));
  auto input = taskparts::cmdline::parse_or_default_string("input", "rmat");
  auto infile = input + ".adj";
  if (n == 0) {
    S = utils::read_symmetric_graph_from_file(infile.c_str());
    n = S.size();
  } else {
    S = utils::rmat_graph(n, 20*n);
  }
#ifndef NDEBUG
  utils::print_graph_stats(S);
#endif
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input2();
  }
  r = set_cover<idx>::run(S, n, epsilon);
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}

int main() {
  taskparts::benchmark_cilk([&] {
    benchmark();
  }, [&] { // setup
    gen_input2();
  }, [&] { // teardown
    if (check(r, S, n)) std::cout << "all elements covered!" << std::endl;
    std::cout << "set_cover_sz " << r.size() << std::endl;
  }, [&] { // reset
    r.clear();
  });
  return 0;
}
