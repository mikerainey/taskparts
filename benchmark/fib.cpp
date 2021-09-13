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

#ifdef TASKPARTS_LOG
using bench_logging = logging_base<true>;
#else
using bench_logging = logging_base<false>;
#endif  
  
auto bench_fib() {
  int64_t n = 30;
  int64_t dst;
  bench_logging::initialize(false, true, false, true);  
  using my_scheduler = taskparts::minimal_scheduler<minimal_stats, bench_logging>;
  auto body = [&] {
    dst = fib_nativefj(n);
  };
  nativefj_from_lambda<decltype(body), my_scheduler> f_body(body);
  auto f_term = new taskparts::terminal_fiber<my_scheduler>;
  taskparts::fiber<my_scheduler>::add_edge(&f_body, f_term);
  f_body.release();
  f_term->release();
  taskparts::chase_lev_work_stealing_scheduler<my_scheduler, taskparts::fiber, minimal_stats, bench_logging>::launch();
  bench_logging::output();
}

} // end namespace

int main() {
  taskparts::bench_fib();
  return 0;
}
