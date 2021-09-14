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
  int n = 40;
  int64_t dst;
  my_logging::initialize(false, true, false, true);  
  using my_scheduler = minimal_scheduler<minimal_stats, my_logging>;
  auto f_body = new fib_par<my_scheduler>(n, &dst);
  auto f_term = new terminal_fiber<my_scheduler>;
  fiber<my_scheduler>::add_edge(f_body, f_term);
  f_body->release();
  f_term->release();
  chase_lev_work_stealing_scheduler<my_scheduler, fiber, minimal_stats, my_logging>::launch();
  my_logging::output();
  return (dst == fib_seq(n));
}

auto test_fib2() -> bool {
  int64_t n = 40;
  int64_t dst;
  my_logging::initialize(false, true, false, true);  
  using my_scheduler = minimal_scheduler<minimal_stats, my_logging>;
  nativefj_from_lambda f_body([&] {
    dst = fib_nativefj(n, my_scheduler());
    printf("dst=%lu\n",dst);
  }, my_scheduler());
  auto f_term = new terminal_fiber<my_scheduler>;
  fiber<my_scheduler>::add_edge(&f_body, f_term);
  f_body.release();
  f_term->release();
  chase_lev_work_stealing_scheduler<my_scheduler, fiber, minimal_stats, my_logging>::launch();
  return (dst == fib_seq(n));
}

auto test_fib3() -> bool {
  int n = 40;
  int64_t dst;
  using my_scheduler = minimal_scheduler<>;
  auto body = [&] {
    dst = fib_oracleguided(n);
  };
  nativefj_from_lambda<decltype(body), my_scheduler> f_body(body);
  auto f_term = new terminal_fiber<my_scheduler>;
  fiber<my_scheduler>::add_edge(&f_body, f_term);
  f_body.release();
  f_term->release();
  chase_lev_work_stealing_scheduler<my_scheduler, fiber>::launch();
  return (dst == fib_seq(n));
}

} // end namespace

int main() {
  //taskparts::test1();
  //printf("fib1 %d\n", taskparts::test_fib1());
  printf("fib2 %d\n", taskparts::test_fib2());
  //  taskparts::test_fib3();
  return 0;
}
