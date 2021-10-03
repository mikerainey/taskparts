#ifndef TASKPARTS_TPALRTS
#error "need to compile with tpal flags, e.g., TASKPARTS_TPALRTS"
#endif
#include <vector>
#include <tuple>
#include <functional>
#include <stdio.h>

#include "sum_tree.hpp"

int main() {
  taskparts::initialize_rollforward();

  node* n0;

  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    taskparts::nativefj_fiber<decltype(sched)>::fork1join(new task([&] {
      auto _f = taskparts::nativefj_fiber<decltype(sched)>::current_fiber.mine();
      std::vector<vhbkont> k({vhbkont(K3, new task([=] {
	_f->release();
      }))});
      sum_heartbeat(n0, k, nullprml, nullprml);
      //sum_serial(n0);
    }));
  }, [&] (auto sched) {
    int h = taskparts::cmdline::parse_or_default_int("h", 28);
    n0 = gen_perfect_tree(h, sched);
  }, [&] (auto sched) {
    delete [] n0;
  });
  
  printf("answer=%d\n", answer);
    
    /*
  auto ns = std::vector<node*>(
    {
      nullptr,
      new node(123),
      new node(1, new node(2), nullptr),
      new node(1, nullptr, new node(2)),  
      new node(1, new node(200), new node(3)),
      new node(1, new node(2), new node(3, new node(4), new node(5))),
      new node(1, new node(3, new node(4), new node(5)), new node(2)),
    });
  for (auto n : ns) {
    answer = -1;

    taskparts::benchmark_nativeforkjoin([&] (auto sched) {
      taskparts::nativefj_fiber<decltype(sched)>::fork1join(new task([&] {
	std::vector<vhbkont> k({vhbkont(K3)});
	//sum_heartbeat(n, k, nullprml, nullprml);
	sum_serial(n);
      }));
    });

    printf("answer=%d\n", answer);
  }
    */
  
  return 0;
}
