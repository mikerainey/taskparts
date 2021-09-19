#include "taskparts/benchmark.hpp"

#include "sum_array_rollforward_decls.hpp"

namespace taskparts {
using scheduler = minimal_scheduler<bench_stats, bench_logging, bench_elastic, bench_worker, bench_interrupt>;
} // end name

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
  taskparts::nativefj_from_lambda<decltype(f1), taskparts::scheduler> fb1(f1);
  taskparts::nativefj_from_lambda<decltype(f2), taskparts::scheduler> fb2(f2);
  taskparts::nativefj_from_lambda<decltype(fj), taskparts::scheduler> fbj(fj);
  auto cfb = taskparts::nativefj_fiber<taskparts::scheduler>::current_fiber.mine();
  cfb->status = taskparts::fiber_status_pause;
  taskparts::fiber<taskparts::scheduler>::add_edge(&fb1, &fbj);
  taskparts::fiber<taskparts::scheduler>::add_edge(&fb2, &fbj);
  taskparts::fiber<taskparts::scheduler>::add_edge(&fbj, cfb);
  fb1.release();
  fb2.release();
  fbj.release();
  if (taskparts::context::capture<taskparts::nativefj_fiber<taskparts::scheduler>*>(taskparts::context::addr(cfb->ctx))) {
    return 1;
  }
  fb1.stack = taskparts::nativefj_fiber<taskparts::scheduler>::notownstackptr;
  fb1.swap_with_scheduler();
  fb1.run();
  auto f = taskparts::scheduler::take<taskparts::nativefj_fiber>();
  if (f == nullptr) {
    cfb->status = taskparts::fiber_status_finish;
    cfb->exit_to_scheduler();
    return 1; // unreachable
  }
  fb2.stack = taskparts::nativefj_fiber<taskparts::scheduler>::notownstackptr;
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
  uint64_t nb_items = 100 * 1000 * 1000;
  double result = 0.0;
  double* a = new double[nb_items];
  for (size_t i = 0; i < nb_items; i++) {
    a[i] = 1.0;
  }

  taskparts::benchmark_nativeforkjoin([&] (auto sched){
    sum_array_heartbeat(a, 0, nb_items, 0.0, &result);
  });
  
  printf("result=%f\n",result);
  delete [] a;
  return 0;
}
