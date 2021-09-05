#include "taskparts/scheduler.hpp"
#include "taskparts/stats.hpp"
#include "taskparts/chaselev.hpp"
#include "../example/fib1.hpp"

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
  
perworker::array<int> pwints;

auto test1() -> bool {
  test_stats ts;
  minimal_worker mw;
  for (int i = 0; i < pwints.size(); i++) {
    pwints[i] = 123;
  }
  return true;
}

auto test_fib() -> bool {
  int n = fib_T * 2;
  int64_t dst;
  using my_scheduler = taskparts::minimal_scheduler<>;
  auto nb_workers = 2;
  taskparts::perworker::id::initialize(nb_workers);
  auto f_body = new fib_fiber<my_scheduler>(n, &dst);
  auto f_term = new taskparts::terminal_fiber<my_scheduler>;
  taskparts::fiber<my_scheduler>::add_edge(f_body, f_term);
  f_body->release();
  f_term->release();
  taskparts::chase_lev_work_stealing_scheduler<my_scheduler, taskparts::fiber>::launch(nb_workers);
  return (dst == fib_seq(n));
}

} // end namespace

int main() {
  assert(taskparts::test1());
  assert(taskparts::test_fib());
  return 0;
}
