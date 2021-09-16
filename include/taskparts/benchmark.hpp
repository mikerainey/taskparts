#pragma once

#include <stdio.h>

#include "taskparts/machine.hpp"
#include "taskparts/chaselev.hpp"
#include "taskparts/stats.hpp"
#include "taskparts/logging.hpp"
#include "taskparts/elastic.hpp"

namespace taskparts {

#ifdef TASKPARTS_STATS
class stats_configuration {
public:

  static constexpr
  bool enabled = true;

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
#else
using bench_stats = minimal_stats;
#endif

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
  
template <typename Benchmark,
	  typename Bench_stats=bench_stats,
	  typename Bench_logging=bench_logging,
	  template <typename, typename> typename Elastic=bench_elastic,
	  typename Scheduler=minimal_scheduler<Bench_stats, Bench_logging, Elastic>>
auto benchmark_nativeforkjoin(const Benchmark& benchmark,
			      Bench_stats stats=Bench_stats(),
			      Bench_logging logging=Bench_logging(),
			      Elastic<Bench_stats, Bench_logging> elastic=Elastic<Bench_stats, Bench_logging>(),
			      Scheduler sched=Scheduler()) {
  size_t repeat = 1;
  if (const auto env_p = std::getenv("TASKPARTS_BENCHMARK_NUM_REPEAT")) {
    repeat = std::stoi(env_p);
  }
  size_t warmup_secs = 3;
  if (const auto env_p = std::getenv("TASKPARTS_BENCHMARK_WARMUP_SECS")) {
    warmup_secs = std::stoi(env_p);
  }
  auto timed_run = [&] {
    auto st = cycles::now();
    benchmark(sched);
    return cycles::seconds_since(st);
  };
  initialize_machine();
  Bench_logging::initialize();
  Bench_stats::start_collecting();
  nativefj_from_lambda f_body([&] {
    if (warmup_secs >= 1) {
      printf("======== WARMUP ========\n");
      auto warmup_start = cycles::now();
      while (cycles::seconds_since(warmup_start).whole_part < warmup_secs) {
	auto el = timed_run();
	printf("warmup_run %lu.%lu\n", el.whole_part, el.fractional_part);
      }
      printf ("======== END WARMUP ========\n");
    }
    for (size_t i = 0; i < repeat; i++) {
      reset([&] {
	Bench_stats::on_enter_work();
      }, [&] {
	Bench_logging::reset();
	Bench_stats::start_collecting();
      }, sched);
      auto el = timed_run();
      reset([&] {
	Bench_stats::on_exit_work();
      }, [&] {
	Bench_stats::output_summary();
	Bench_logging::output("log" + std::to_string(i) + ".json");
	Bench_stats::start_collecting();
      }, sched);
      printf("exectime %lu.%lu\n", el.whole_part, el.fractional_part);
      if (i + 1 != repeat) {
	printf("---\n");
      }
    }
  }, sched);
  auto f_term = new terminal_fiber<Scheduler>;
  fiber<Scheduler>::add_edge(&f_body, f_term);
  f_body.release();
  f_term->release();
  using cl = chase_lev_work_stealing_scheduler<Scheduler, fiber, Bench_stats, Bench_logging, Elastic>;
  cl::launch();
  teardown_machine();
}

} // end namespace
