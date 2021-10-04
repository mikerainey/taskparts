#pragma once

#include <string>
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#include <taskparts/machine.hpp>
#include <taskparts/stats.hpp>
#include <taskparts/cmdline.hpp>

namespace taskparts {

class stats_configuration {
public:

  static constexpr
  bool collect_all_stats = false;
  
  using counter_id_type = enum counter_id_enum {
    nb_fibers,
    nb_steals,
    nb_counters
  };

  static
  auto name_of_counter(counter_id_type id) -> const char* {
    const char* names [] = { "nb_fibers", "nb_steals" };
    return names[id];
  }
  
};
  
using bench_stats = stats_base<stats_configuration>;

auto dflt_benchmark_setup = [] { };
auto dflt_benchmark_teardown = []  { };
auto dflt_benchmark_reset = [] { };
  
template <typename Benchmark,
	  typename Benchmark_setup=decltype(dflt_benchmark_setup),
	  typename Benchmark_teardown=decltype(dflt_benchmark_teardown),
	  typename Benchmark_reset=decltype(dflt_benchmark_reset)
>
auto benchmark_cilk(const Benchmark& benchmark,
		    Benchmark_setup benchmark_setup=dflt_benchmark_setup,
		    Benchmark_teardown benchmark_teardown=dflt_benchmark_teardown,
		    Benchmark_reset benchmark_reset=dflt_benchmark_reset) {
  size_t repeat = 1;
  if (const auto env_p = std::getenv("TASKPARTS_BENCHMARK_NUM_REPEAT")) {
    repeat = std::stoi(env_p);
  }
  double warmup_secs = 3.0;
  if (const auto env_p = std::getenv("TASKPARTS_BENCHMARK_WARMUP_SECS")) {
    warmup_secs = (double)std::stoi(env_p);
  }
  bool verbose = false;
  if (const auto env_p = std::getenv("TASKPARTS_BENCHMARK_VERBOSE")) {
    verbose = std::stoi(env_p);
  }
  initialize_machine();
  size_t nb_workers = perworker::nb_workers_requested();
  int cilk_failed = __cilkrts_set_param("nworkers", std::to_string(nb_workers).c_str());
  if (cilk_failed) {
    taskparts_die("Failed to set number of processors to %d in Cilk runtime", nb_workers);
  }
  benchmark_setup();
  if (warmup_secs >= 1.0) {
    if (verbose) printf("======== WARMUP ========\n");
    auto warmup_start = steadyclock::now();
    while (steadyclock::since(warmup_start) < warmup_secs) {
      auto st = steadyclock::now();
      benchmark();
      auto el = steadyclock::since(st);
      if (verbose) printf("warmup_run %.3f\n", el);
      benchmark_reset();
    }
    if (verbose) printf ("======== END WARMUP ========\n");
  }
  for (size_t i = 0; i < repeat; i++) {
    bench_stats::start_collecting();
    benchmark();
    bench_stats::capture_summary();
    if (i + 1 < repeat) {
      benchmark_reset();
    }
    benchmark_teardown();
  }
  bench_stats::output_summaries();
  teardown_machine();
}
  
} // end namespace
