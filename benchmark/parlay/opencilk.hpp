#pragma once

#include <string>
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
//#include <cilk/reducer_opadd.h>
#include <taskparts/machine.hpp>
#include <taskparts/stats.hpp>
#include <taskparts/cmdline.hpp>
#include <taskparts/defaults.hpp>

namespace taskparts {

#ifdef TASKPARTS_CILKRTS_WITH_STATS
void trigger_cilk() {
  printf("");
}
#endif

  /*
__attribute__((constructor))
void init_cilkrts() {
  size_t nb_workers = perworker::nb_workers_requested();
  int cilk_failed = __cilkrts_set_param("nworkers", std::to_string((int)nb_workers).c_str());
  if (cilk_failed) {
    taskparts_die("Failed to set number of processors to %d in Cilk runtime", nb_workers);
  }
}
  */
auto dflt_cilk_benchmark_setup = [] { };
auto dflt_cilk_benchmark_teardown = []  { };
auto dflt_cilk_benchmark_reset = [] { };
  
template <typename Benchmark,
	  typename Benchmark_setup=decltype(dflt_cilk_benchmark_setup),
	  typename Benchmark_teardown=decltype(dflt_cilk_benchmark_teardown),
	  typename Benchmark_reset=decltype(dflt_cilk_benchmark_reset)
>
auto benchmark_cilk(const Benchmark& benchmark,
		    Benchmark_setup benchmark_setup=dflt_cilk_benchmark_setup,
		    Benchmark_teardown benchmark_teardown=dflt_cilk_benchmark_teardown,
		    Benchmark_reset benchmark_reset=dflt_cilk_benchmark_reset) {
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
  size_t nb_workers = perworker::nb_workers_requested();
  initialize_machine();
#ifdef TASKPARTS_CILKRTS_WITH_STATS
  cilk_spawn trigger_cilk();
  cilk_sync;
#endif
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
#ifdef TASKPARTS_CILKRTS_WITH_STATS
  std::string outfile = "";
  if (const auto env_p = std::getenv("CILK_STATS_OUTFILE")) {
    outfile = std::string(env_p);
  }
  FILE* f = (outfile == "") ? stdout : fopen(outfile.c_str(), "w");
  fprintf(f, "[");
#endif
  for (size_t i = 0; i < repeat; i++) {
#ifdef TASKPARTS_CILKRTS_WITH_STATS
    __cilkg_take_snapshot_for_stats();
#endif
    bench_stats::start_collecting();
    benchmark();
    bench_stats::capture_summary();
#ifdef TASKPARTS_CILKRTS_WITH_STATS
    __cilkg_dump_json_stats_to_file(f, get_cpu_frequency_khz());
#endif
    if (i + 1 < repeat) {
      benchmark_reset();
#ifdef TASKPARTS_CILKRTS_WITH_STATS
      fprintf(f, ",");
#endif
    }
  }
  benchmark_teardown();
#ifdef TASKPARTS_CILKRTS_WITH_STATS
  fprintf(f, "]\n");
  if (f != stdout) {
    fclose(f);
  }
#endif
  bench_stats::output_summaries();
  teardown_machine();
}
  
} // end namespace
