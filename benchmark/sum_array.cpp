#ifndef TASKPARTS_TPALRTS
#error "need to compile with tpal flags, e.g., TASKPARTS_TPALRTS"
#endif
#include <taskparts/benchmark.hpp>
#include "sum_array_rollforward_decls.hpp"

void sum_array_heartbeat(double* a, uint64_t lo, uint64_t hi, double r, double* dst);

int sum_array_heartbeat_handler(double* a, uint64_t lo, uint64_t hi, double r, double* dst) {
  if ((hi - lo) <= 1) {
    return 0;
  }
  double dst1, dst2;
  auto mid = (lo + hi) / 2;
  auto f1 = [&] {
    sum_array_heartbeat(a, lo, mid, 0.0, &dst1);
  };
  auto f2 = [&] {
    sum_array_heartbeat(a, mid, hi, 0.0, &dst2);
  };
  auto fj = [&] {
    *dst = r + dst1 + dst2;
  };
  taskparts::nativefj_from_lambda<decltype(f1), taskparts::bench_scheduler> fb1(f1);
  taskparts::nativefj_from_lambda<decltype(f2), taskparts::bench_scheduler> fb2(f2);
  taskparts::nativefj_from_lambda<decltype(fj), taskparts::bench_scheduler> fbj(fj);
  auto cfb = taskparts::nativefj_fiber<taskparts::bench_scheduler>::current_fiber.mine();
  cfb->status = taskparts::fiber_status_pause;
  taskparts::fiber<taskparts::bench_scheduler>::add_edge(&fb1, &fbj);
  taskparts::fiber<taskparts::bench_scheduler>::add_edge(&fb2, &fbj);
  taskparts::fiber<taskparts::bench_scheduler>::add_edge(&fbj, cfb);
  fb1.release();
  fb2.release();
  fbj.release();
  if (taskparts::context::capture<taskparts::nativefj_fiber<taskparts::bench_scheduler>*>(taskparts::context::addr(cfb->ctx))) {
    return 1;
  }
  fb1.stack = taskparts::nativefj_fiber<taskparts::bench_scheduler>::notownstackptr;
  fb1.swap_with_scheduler();
  fb1.run();
  auto f = taskparts::bench_scheduler::take<taskparts::nativefj_fiber>();
  if (f == nullptr) {
    cfb->status = taskparts::fiber_status_finish;
    cfb->exit_to_scheduler();
    return 1; // unreachable
  }
  fb2.stack = taskparts::nativefj_fiber<taskparts::bench_scheduler>::notownstackptr;
  fb2.swap_with_scheduler();
  fb2.run();
  cfb->status = taskparts::fiber_status_finish;
  cfb->swap_with_scheduler();
  return 1;
}

namespace taskparts {
auto initialize_rollforward() {
  rollforward_table = {
    #include "sum_array_rollforward_map.hpp"
  };
  initialize_rollfoward_table();
}
} // end namespace

int main() {
  taskparts::initialize_rollforward();
  uint64_t n = taskparts::cmdline::parse_or_default_long("n", 100 * 1000 * 1000);
  double result = 0.0;
  double* a;

  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    sum_array_heartbeat(a, 0, n, 0.0, &result);
  }, [&] (auto sched) {
    a = new double[n];
    taskparts::parallel_for(0, n, [&] (auto i) {
      a[i] = 1.0;
    }, sched);
  }, [&] (auto sched) {
    delete [] a;
  });
  
  printf("result=%f\n",result);
  return 0;
}
