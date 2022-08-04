#include "common.hpp"
#include "cartesian_tree.h"

parlay::sequence<long> generate_values(long n) {
  parlay::random_generator gen;
  std::uniform_int_distribution<long> dis(0, n-1);

  return parlay::tabulate(n, [&] (long i) {
    auto r = gen[i];
    return dis(r);
  });
}

parlay::sequence<long> values;
parlay::sequence<long> parents;
long n;

auto gen_input() {
  n = std::max((size_t)1, (size_t)taskparts::cmdline::parse_or_default_long("n", 10 * 1000 * 1000));
  values = generate_values(n);
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  parents = cartesian_tree(values);
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}

int main() {
  parlay::benchmark_taskparts([&] (auto sched) { // benchmark
    benchmark();
  }, [&] (auto sched) { // setup
    gen_input();
  }, [&] (auto sched) { // teardown
#ifndef NDEBUG
    auto h = parlay::tabulate(n, [&] (long i) {
      long depth = 1;
      while (i != parents[i]) {
        i = parents[i];
        depth++;
      }
      return depth;});
    
    std::cout << "depth of tree: "
              << parlay::reduce(h, parlay::maximum<long>())
              << std::endl;
#endif
  }, [&] (auto sched) { // reset

  });
  return 0;
}
