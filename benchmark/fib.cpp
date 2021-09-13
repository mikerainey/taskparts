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
  
auto test_fib3() -> bool {
  int n = fib_T * 2;
  int64_t dst;
  using my_scheduler = taskparts::minimal_scheduler<>;
  auto body = [&] {
    dst = fib_oracleguided(n);
  };
  nativefj_from_lambda<decltype(body), my_scheduler> f_body(body);
  auto f_term = new taskparts::terminal_fiber<my_scheduler>;
  taskparts::fiber<my_scheduler>::add_edge(&f_body, f_term);
  f_body.release();
  f_term->release();
  taskparts::chase_lev_work_stealing_scheduler<my_scheduler, taskparts::fiber>::launch();
  return (dst == fib_seq(n));
}

} // end namespace

int main() {
  taskparts::test_fib3();
  return 0;
}
