#include <stdio.h>
#include <assert.h>

#include "../../example/mixed.hpp"
#include <taskparts/benchmark.hpp>

int main() {
  nb_seq_iters = taskparts::cmdline::parse_or_default_long("nb_seq_iters", 1<<20);
  nb_par_iters = taskparts::cmdline::parse_or_default_long("nb_par_iters", 1<<20);
  auto nb_workers = taskparts::perworker::nb_workers();
  nb_par_paths = taskparts::cmdline::parse_or_default_long("nb_par_paths", nb_workers);
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    taskparts::fork1fiberjoin(new alternate(sched));
  });
  printf("nb_iterations %lu\n", nb_iterations);
  return 0;
}
