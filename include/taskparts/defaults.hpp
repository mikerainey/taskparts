#pragma once

#include "machine.hpp"
#include "taskparts/stats.hpp"
#include "taskparts/logging.hpp"
#include "taskparts/elastic.hpp"
#include "taskparts/nativeforkjoin.hpp"
#include "taskparts/workstealing.hpp"

#ifdef TASKPARTS_TPALRTS

using ping_thread_status_type = enum ping_thread_status_enum {
  ping_thread_status_active,
  ping_thread_status_exit_launch,
  ping_thread_status_exited
};

using promote_status_type = enum promote_status_enum {
  tpal_promote_fail = 0,
  tpal_promote_success = 1
};

#if defined(TASKPARTS_POSIX)
#include "posix/tpalrts.hpp"
#elif defined(TASKPARTS_DARWIN)

#elif defined (TASKPARTS_NAUTILUS)
#include "nautilus/tpalrts.hpp"
#else
#error need to declare platform (e.g., TASKPARTS_POSIX)
#endif

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
#if defined(TASKPARTS_ELASTIC_SURPLUS)
    nb_sleeps, nb_surplus_transitions,
#endif
    nb_counters
  };

  static
  auto name_of_counter(counter_id_type id) -> const char* {
    const char* names [] = { "nb_fibers", "nb_steals",
#if defined(TASKPARTS_ELASTIC_SURPLUS)
			     "nb_sleeps", "nb_surplus_transitions"
#endif
    };
    return names[id];
  }
  
};
  
using bench_stats = stats_base<stats_configuration>;

#ifdef TASKPARTS_LOG
using bench_logging = logging_base<true>;
#else
using bench_logging = logging_base<false>;
#endif

#ifdef TASKPARTS_TPALRTS
using bench_worker = tpalrts_worker;
using bench_interrupt = tpalrts_interrupt;
#else
using bench_worker = minimal_worker;
using bench_interrupt = minimal_interrupt;
#endif

using bench_scheduler = minimal_scheduler<bench_stats, bench_logging, 
					  bench_worker, bench_interrupt>;

auto dflt_benchmark_setup = [] (bench_scheduler) { };
auto dflt_benchmark_teardown = [] (bench_scheduler) { };
auto dflt_benchmark_reset = [] (bench_scheduler) { };

template<typename F>
auto launch(const F& f) {
  using scheduler_type = minimal_scheduler<>;
  scheduler_type sched;
  auto run = [&] {
    f(sched);
  };
  initialize_machine();
  nativefj_from_lambda f_body(run, sched);
  auto f_term = new terminal_fiber<scheduler_type>;
  fiber<scheduler_type>::add_edge(&f_body, f_term);
  f_body.release();
  f_term->release();
  using cl = work_stealing<scheduler_type, fiber>;
  cl::launch();
  teardown_machine();
}
  
} // end namespace
