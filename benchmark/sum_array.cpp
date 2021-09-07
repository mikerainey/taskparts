#include "taskparts/chaselev.hpp"
#include "taskparts/tpalrts.hpp"
#include "taskparts/nativeforkjoin.hpp"

#include "sum_array_rollforward_decls.hpp"

namespace taskparts {
using tpalrts_scheduler = minimal_scheduler<minimal_stats, minimal_logging, minimal_elastic,
					    ping_thread_worker, ping_thread_interrupt>;
using my_scheduler = chase_lev_work_stealing_scheduler<tpalrts_scheduler, fiber,
						       minimal_stats, minimal_logging, minimal_elastic,
						       ping_thread_worker, ping_thread_interrupt>;
}

double sum_array_serial(double* a, uint64_t lo, uint64_t hi);

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
  taskparts::nativefj_from_lambda<decltype(f1), taskparts::tpalrts_scheduler> fb1(f1);
  taskparts::nativefj_from_lambda<decltype(f2), taskparts::tpalrts_scheduler> fb2(f2);
  taskparts::nativefj_from_lambda<decltype(fj), taskparts::tpalrts_scheduler> fbj(fj);
  auto cfb = taskparts::nativefj_fiber<taskparts::tpalrts_scheduler>::current_fiber.mine();
  cfb->status = taskparts::fiber_status_pause;
  taskparts::fiber<taskparts::tpalrts_scheduler>::add_edge(&fb1, &fbj);
  taskparts::fiber<taskparts::tpalrts_scheduler>::add_edge(&fb2, &fbj);
  taskparts::fiber<taskparts::tpalrts_scheduler>::add_edge(&fbj, cfb);
  fb1.release();
  fb2.release();
  fbj.release();
  if (taskparts::context::capture<taskparts::nativefj_fiber<taskparts::tpalrts_scheduler>*>(taskparts::context::addr(cfb->ctx))) {
    return 1;
  }
  fb1.stack = taskparts::nativefj_fiber<taskparts::tpalrts_scheduler>::notownstackptr;
  fb1.swap_with_scheduler();
  fb1.run();
  auto f = taskparts::tpalrts_scheduler::take<taskparts::nativefj_fiber>();
  if (f == nullptr) {
    cfb->status = taskparts::fiber_status_finish;
    cfb->exit_to_scheduler();
    return 1; // unreachable
  }
  fb2.stack = taskparts::nativefj_fiber<taskparts::tpalrts_scheduler>::notownstackptr;
  fb2.swap_with_scheduler();
  fb2.run();
  cfb->status = taskparts::fiber_status_finish;
  cfb->swap_with_scheduler();
  return 1;
}

uint64_t nb_items = 100 * 1000 * 1000;
double* a;
double result = 0.0;

namespace taskparts {

void init(size_t nb_workers) {
  uint64_t my_cpu_freq_khz = 3500000;
  assign_kappa(my_cpu_freq_khz);
  taskparts::perworker::id::initialize(nb_workers);
  rollforward_table = {
    #include "sum_array_rollforward_map.hpp"
  };
  initialize_rollfoward_table();
}

} // end namespace

int main() {
  double* a = new double[nb_items];
  for (size_t i = 0; i < nb_items; i++) {
    a[i] = 1.0;
  }

  size_t nb_workers = 15;
  taskparts::init(nb_workers);
  auto f0 = [&] {
    sum_array_heartbeat(a, 0, nb_items, 0.0, &result);
  };
  taskparts::nativefj_from_lambda<decltype(f0), taskparts::tpalrts_scheduler> f_body(f0);
  auto f_term = new taskparts::terminal_fiber<taskparts::tpalrts_scheduler>;
  taskparts::fiber<taskparts::tpalrts_scheduler>::add_edge(&f_body, f_term);
  f_body.release();
  f_term->release();
  taskparts::my_scheduler::launch(nb_workers);
  
  printf("result=%f\n",result);
  delete [] a;
  return 0;
}
