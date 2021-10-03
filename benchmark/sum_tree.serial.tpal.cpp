 #include <vector>
#include <stdio.h>

#include <taskparts/benchmark.hpp>
#include "sum_tree.hpp"

void sum_serial(node* n);

int main() {
  
  node* n0;

  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    taskparts::nativefj_fiber<decltype(sched)>::fork1join(new task([&] {
      auto _f = taskparts::nativefj_fiber<decltype(sched)>::current_fiber.mine();
      sum_serial(n0);
      _f->release();
    }));
  }, [&] (auto sched) {
    int h = taskparts::cmdline::parse_or_default_int("h", 28);
    n0 = gen_perfect_tree(h, sched);
  }, [&] (auto sched) {
    delete [] n0;
  });
  
  printf("answer=%d\n", answer);
  return 0;
}
