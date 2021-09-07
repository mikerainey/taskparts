#include "taskparts/chaselev.hpp"
#include "taskparts/tpalrts.hpp"
#include "taskparts/nativeforkjoin.hpp"

#include "sum_array_rollforward_decls.hpp"

namespace taskparts {
using tpalrts_scheduler = minimal_scheduler<minimal_stats, minimal_logging, minimal_elastic,
					    ping_thread_worker, ping_thread_interrupt>;
using my_scheduler = chase_lev_work_stealing_scheduler<tpalrts_scheduler, fiber,
						       minimal_stats, minimal_logging, minimal_elastic,
						       ping_thread_worker>;
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
  
  return 1;
}

uint64_t nb_items = 100 * 1000 * 1000;
double* a;
double result = 0.0;

namespace taskparts {
  // todo: complete initalization, e.g., init_print_lock(), from mcsl_machine initialize_machine
void init(size_t nb_workers) {
  taskparts::rollforward_table = {
    #include "sum_array_rollforward_map.hpp"
  };
  uint64_t my_cpu_freq_khz = 3500000;
  assign_kappa(my_cpu_freq_khz);
  taskparts::perworker::id::initialize(nb_workers);
}
}

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
  return 0;
}
