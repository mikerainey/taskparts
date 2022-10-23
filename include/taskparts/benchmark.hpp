#pragma once

#include <stdio.h>

#include "taskparts/defaults.hpp"
#include "taskparts/chaselev.hpp"
#include "taskparts/oracleguided.hpp"
#include "taskparts/cmdline.hpp"
#ifdef TASKPARTS_TPALRTS
#include "taskparts/tpalrts.hpp"
#endif

namespace taskparts {

template <typename Benchmark,
	  typename Benchmark_setup=decltype(dflt_benchmark_setup),
	  typename Benchmark_teardown=decltype(dflt_benchmark_teardown),
	  typename Benchmark_reset=decltype(dflt_benchmark_reset),
	  typename Bench_stats=bench_stats,
	  typename Bench_logging=bench_logging,
	  template <typename, typename> typename Bench_elastic=bench_elastic,
	  typename Bench_worker=bench_worker,
	  typename Bench_interrupt=bench_interrupt,
	  typename Scheduler=minimal_scheduler<Bench_stats, Bench_logging, Bench_elastic, Bench_worker, Bench_interrupt>>
auto benchmark_nativeforkjoin(const Benchmark& benchmark,
			      Benchmark_setup benchmark_setup=dflt_benchmark_setup,
			      Benchmark_teardown benchmark_teardown=dflt_benchmark_teardown,
			      Benchmark_reset benchmark_reset=dflt_benchmark_reset,
			      Bench_stats stats=Bench_stats(),
			      Bench_logging logging=Bench_logging(),
			      Bench_elastic<Bench_stats, Bench_logging> elastic=Bench_elastic<Bench_stats, Bench_logging>(),
			      Bench_worker worker=Bench_worker(),
			      Bench_interrupt interrupt=Bench_interrupt(),
			      Scheduler sched=Scheduler()) {
  // runtime parameters
  size_t repeat = 1;
  if (const auto env_p = std::getenv("TASKPARTS_BENCHMARK_NUM_REPEAT")) {
    repeat = std::stoi(env_p);
  }
#ifdef NDEBUG
  double warmup_secs = 3.0;
#else
  double warmup_secs = 0.0;
#endif
  if (const auto env_p = std::getenv("TASKPARTS_BENCHMARK_WARMUP_SECS")) {
    warmup_secs = (double)std::stoi(env_p);
  }
  bool verbose = false;
  if (const auto env_p = std::getenv("TASKPARTS_BENCHMARK_VERBOSE")) {
    verbose = std::stoi(env_p);
  }
  // initialization
  initialize_machine();
#ifdef TASKPARTS_TPALRTS
  initialize_tpalrts();
#endif
  Bench_logging::initialize();
  Bench_stats::start_collecting();
  // warmup
  auto warmup = [&] {
    if (warmup_secs >= 1.0) {
      if (verbose) printf("======== WARMUP ========\n");
      auto warmup_start = steadyclock::now();
      while (steadyclock::since(warmup_start) < warmup_secs) {
	auto st = steadyclock::now();
	benchmark(sched);
	auto el = steadyclock::since(st);
	if (verbose) printf("warmup_run %.3f\n", el);
	benchmark_reset(sched);
      }
      if (verbose) printf ("======== END WARMUP ========\n");
    }
  };
  // benchmark run
  auto run = [&] {
    benchmark_setup(sched);
    warmup();
    for (size_t i = 0; i < repeat; i++) {
      reset([&] {
	Bench_stats::on_enter_work();
      }, [&] {
	Bench_logging::reset();
	Bench_stats::start_collecting();
      }, false, sched);
      benchmark(sched);
      reset([&] {
	Bench_stats::on_exit_work();
      }, [&] {
	Bench_stats::capture_summary();
	Bench_logging::output("log" + std::to_string(i) + ".json");
	if (i + 1 < repeat) {
	  benchmark_reset(sched);
	}
	Bench_stats::start_collecting();
      }, true, sched);
    }
    benchmark_teardown(sched);
  };
#ifndef TASKPARTS_SERIAL
  // parallel run
  nativefj_from_lambda f_body(run, sched);
  auto f_term = new terminal_fiber<Scheduler>;
  fiber<Scheduler>::add_edge(&f_body, f_term);
  f_body.release();
  f_term->release();
  using cl = chase_lev_work_stealing_scheduler<Scheduler, fiber,
					       Bench_stats, Bench_logging,
					       Bench_elastic, Bench_worker, Bench_interrupt>;
  cl::launch();
#else
  // serial run
  run();
#endif
  // teardown
  Bench_stats::output_summaries();
  teardown_machine();
}

} // end namespace
