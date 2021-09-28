#pragma once

#include <stdio.h>

#include "taskparts/machine.hpp"
#include "taskparts/chaselev.hpp"
#include "taskparts/stats.hpp"
#include "taskparts/logging.hpp"
#include "taskparts/elastic.hpp"
#include "taskparts/nativeforkjoin.hpp"
#include "taskparts/cmdline.hpp"
#ifdef TASKPARTS_TPALRTS
#include "taskparts/tpalrts.hpp"
#endif

namespace taskparts {

class stats_configuration {
public:

#ifdef TASKPARTS_STATS
  static constexpr
  bool collect_all_stats = true;
#else
  static constexpr
  bool collect_all_stats = false;
#endif
  
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

#ifdef TASKPARTS_LOG
using bench_logging = logging_base<true>;
#else
using bench_logging = logging_base<false>;
#endif

#ifdef TASKPARTS_ELASTIC
template <typename Stats, typename Logging>
using bench_elastic = elastic<Stats, Logging>;
#else
template <typename Stats, typename Logging>
using bench_elastic = minimal_elastic<Stats, Logging>;
#endif

#ifdef TASKPARTS_TPALRTS
using bench_worker = ping_thread_worker;
using bench_interrupt = ping_thread_interrupt;
#else
using bench_worker = minimal_worker;
using bench_interrupt = minimal_interrupt;
#endif

using bench_scheduler = minimal_scheduler<bench_stats, bench_logging, bench_elastic,
					  bench_worker, bench_interrupt>;

auto dflt_benchmark_setup = [] (bench_scheduler) { };
auto dflt_benchmark_teardown = [] (bench_scheduler) { };
  
template <typename Benchmark,
	  typename Benchmark_setup=decltype(dflt_benchmark_setup),
	  typename Benchmark_teardown=decltype(dflt_benchmark_teardown),
	  typename Bench_stats=bench_stats,
	  typename Bench_logging=bench_logging,
	  template <typename, typename> typename Bench_elastic=bench_elastic,
	  typename Bench_worker=bench_worker,
	  typename Bench_interrupt=bench_interrupt,
	  typename Scheduler=minimal_scheduler<Bench_stats, Bench_logging, Bench_elastic, Bench_worker, Bench_interrupt>>
auto benchmark_nativeforkjoin(const Benchmark& benchmark,
			      Benchmark_setup benchmark_setup=dflt_benchmark_setup,
			      Benchmark_teardown benchmark_teardown=dflt_benchmark_teardown,
			      Bench_stats stats=Bench_stats(),
			      Bench_logging logging=Bench_logging(),
			      Bench_elastic<Bench_stats, Bench_logging> elastic=Bench_elastic<Bench_stats, Bench_logging>(),
			      Bench_worker worker=Bench_worker(),
			      Bench_interrupt interrupt=Bench_interrupt(),
			      Scheduler sched=Scheduler()) {
  size_t repeat = 1;
  if (const auto env_p = std::getenv("TASKPARTS_BENCHMARK_NUM_REPEAT")) {
    repeat = std::stoi(env_p);
  }
  size_t warmup_secs = 3;
  if (const auto env_p = std::getenv("TASKPARTS_BENCHMARK_WARMUP_SECS")) {
    warmup_secs = std::stoi(env_p);
  }
  bool verbose = false;
  if (const auto env_p = std::getenv("TASKPARTS_BENCHMARK_VERBOSE")) {
    verbose = std::stoi(env_p);
  }
  initialize_machine();
  Bench_logging::initialize();
  Bench_stats::start_collecting();
  nativefj_from_lambda f_body([&] {
    benchmark_setup(sched);
    if (warmup_secs >= 1) {
      if (verbose) printf("======== WARMUP ========\n");
      auto warmup_start = cycles::now();
      while (cycles::seconds_since(warmup_start).whole_part < warmup_secs) {
	auto st = cycles::now();
	benchmark(sched);
	auto el = cycles::seconds_since(st);
	if (verbose) printf("warmup_run %lu.%lu\n", el.whole_part, el.fractional_part);
      }
      if (verbose) printf ("======== END WARMUP ========\n");
    }
    for (size_t i = 0; i < repeat; i++) {
      reset([&] {
	Bench_stats::on_enter_work();
      }, [&] {
	Bench_logging::reset();
	Bench_stats::start_collecting();
      }, sched);
      benchmark(sched);
      reset([&] {
	Bench_stats::on_exit_work();
      }, [&] {
	Bench_stats::capture_summary();
	Bench_logging::output("log" + std::to_string(i) + ".json");
	Bench_stats::start_collecting();
      }, sched);
    }
    benchmark_teardown(sched);
  }, sched);
  auto f_term = new terminal_fiber<Scheduler>;
  fiber<Scheduler>::add_edge(&f_body, f_term);
  f_body.release();
  f_term->release();
  using cl = chase_lev_work_stealing_scheduler<Scheduler, fiber, Bench_stats, Bench_logging, Bench_elastic, Bench_worker, Bench_interrupt>;
  cl::launch();
  Bench_stats::output_summaries();
  teardown_machine();
}

} // end namespace
