#include "taskparts/scheduler.hpp"
#include "taskparts/stats.hpp"
#include "taskparts/chaselev.hpp"
#include "taskparts/machine.hpp"
#include "taskparts/logging.hpp"
#include "../example/fib_par.hpp"
#include "../example/fib_nativefj.hpp"
#include "../example/fib_oracleguided.hpp"

#include <map>
#include <stdio.h>
#include <assert.h>

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
    std::map<counter_id_type, const char*> names;
    names[nb_fibers] = "nb_fibers";
    names[nb_steals] = "nb_steals";
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

auto bench_fib() {
  int64_t n = 40;
  int64_t dst;
  bench_logging::initialize(false, true, false, true);  
  using scheduler = minimal_scheduler<bench_stats, bench_logging>;
  nativefj_from_lambda f_body([&] {
    //dst = fib_nativefj(n, scheduler());
    dst = fib_oracleguided(n, scheduler());
  }, scheduler());
  auto f_term = new terminal_fiber<scheduler>;
  fiber<scheduler>::add_edge(&f_body, f_term);
  f_body.release();
  f_term->release();
  bench_stats::start_collecting();
  chase_lev_work_stealing_scheduler<scheduler, fiber, bench_stats, bench_logging>::launch();
  bench_stats::output_summary();
  bench_logging::output();
}

} // end namespace

int main() {
  taskparts::bench_fib();
  return 0;
}
