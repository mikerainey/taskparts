#pragma once

#include "machine.hpp"
#include "taskparts/stats.hpp"
#include "taskparts/logging.hpp"
#include "taskparts/elastic.hpp"
#include "taskparts/nativeforkjoin.hpp"

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
#elif defined (TASKPARTS_NAUTILUS)
#include "nautilus/tpalrts.hpp"
#else
#error need to declare platform (e.g., TASKPARTS_POSIX)
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

#if defined(TASKPARTS_ELASTIC_LIFELINE)
template <typename Stats, typename Logging>
using bench_elastic = elastic<Stats, Logging>;
#elif defined(TASKPARTS_ELASTIC_FLAT)
template <typename Stats, typename Logging>
using bench_elastic = elastic_flat<Stats, Logging>;
#else
template <typename Stats, typename Logging>
using bench_elastic = minimal_elastic<Stats, Logging>;
#endif

#ifdef TASKPARTS_TPALRTS
using bench_worker = tpalrts_worker;
using bench_interrupt = tpalrts_interrupt;
#else
using bench_worker = minimal_worker;
using bench_interrupt = minimal_interrupt;
#endif

using bench_scheduler = minimal_scheduler<bench_stats, bench_logging, bench_elastic,
					  bench_worker, bench_interrupt>;

auto dflt_benchmark_setup = [] (bench_scheduler) { };
auto dflt_benchmark_teardown = [] (bench_scheduler) { };
auto dflt_benchmark_reset = [] (bench_scheduler) { };

} // end namespace
