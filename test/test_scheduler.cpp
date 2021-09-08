#include "taskparts/scheduler.hpp"
#include "taskparts/stats.hpp"
#include "taskparts/chaselev.hpp"
#include "taskparts/machine.hpp"
#include "taskparts/logging.hpp"
#include "../example/fib1.hpp"
#include "../example/fib2.hpp"

#include <map>
#include <stdio.h>
#include <assert.h>

namespace taskparts {

class my_stats_configuration {
public:

  static constexpr
  bool enabled = true;

  using counter_id_type = enum counter_id_enum {
    nb_fibers,
    nb_steals,
    nb_sleeps,
    nb_counters
  };

  static
  const char* name_of_counter(counter_id_type id) {
    std::map<counter_id_type, const char*> names;
    names[nb_fibers] = "nb_fibers";
    names[nb_steals] = "nb_steals";
    names[nb_sleeps] = "nb_sleeps";
    return names[id];
  }
  
};

using test_stats = stats_base<my_stats_configuration>;

using my_logging = logging_base<true>;
  
perworker::array<int> pwints;

auto test1() -> bool {
  test_stats ts;
  minimal_worker mw;
  for (int i = 0; i < pwints.size(); i++) {
    pwints[i] = 123;
  }
  return true;
}

auto test_fib1() -> bool {
  int n = 41;
  int64_t dst;
  my_logging::initialize(false, true, false, true);  
  using my_scheduler = taskparts::minimal_scheduler<minimal_stats, my_logging>;
  auto f_body = new fib_par<my_scheduler>(n, &dst);
  auto f_term = new taskparts::terminal_fiber<my_scheduler>;
  taskparts::fiber<my_scheduler>::add_edge(f_body, f_term);
  f_body->release();
  f_term->release();
  taskparts::chase_lev_work_stealing_scheduler<my_scheduler, taskparts::fiber, minimal_stats, my_logging>::launch();
  my_logging::output();
  return (dst == fib_seq(n));
}

auto test_fib2() -> bool {
  int n = fib_T * 2;
  int64_t dst;
  using my_scheduler = taskparts::minimal_scheduler<>;
  auto body = [&] {
    dst = fib_par_nativefj(n);
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
  assert(taskparts::test1());
  assert(taskparts::test_fib1());
  assert(taskparts::test_fib2());
  return 0;
}
