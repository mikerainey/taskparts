#include "taskparts/nativeforkjoin.hpp"
#ifndef TASKPARTS_TPALRTS
#error "need to compile with tpal flags, e.g., TASKPARTS_TPALRTS"
#endif
#include <taskparts/benchmark.hpp>

void sum_array_heartbeat(double* a, uint64_t lo, uint64_t hi, double r, double* dst);

/* Handler functions */
/* ================= */

void sum_array_handler(double* a, uint64_t lo, uint64_t hi, double r, double* dst, bool& promoted) {
  if ((hi - lo) <= 1) {
    promoted = false;
  } else {
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
    tpalrts_promote_via_nativefj(f1, f2, fj, taskparts::bench_scheduler());
    promoted = true;
  }
}

void taskparts_tpal_handler __rf_handle_sum_array_heartbeat(double* a, uint64_t lo, uint64_t hi,
							    double r, double* dst, bool& promoted) {
  sum_array_handler(a, lo, hi, r, dst, promoted);
  taskparts_tpal_rollbackward
}

/* Outlined-loop functions */
/* ======================= */

#define D 64

void sum_array_heartbeat(double* a, uint64_t lo, uint64_t hi, double r, double* dst) {
  if (! (lo < hi)) {
    goto exit;
  }
  for (; ; ) {
    uint64_t lo2 = lo;
    uint64_t hi2 = std::min(lo + D, hi);
    for (; lo2 < hi2; lo2++) {
      r += a[lo2];
    }
    lo = lo2;
    if (! (lo < hi)) {
      break;
    }
    bool promoted = false;
    __rf_handle_sum_array_heartbeat(a, lo, hi, r, dst, promoted);
    if (taskparts_tpal_unlikely(promoted)) {
      return;
    }
  }
 exit:
  *dst = r;
}

int main() {
  uint64_t n = taskparts::cmdline::parse_or_default_long("n", 1000 * 1000 * 1000);
  double result = 0.0;
  double* a;
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    sum_array_heartbeat(a, 0, n, 0.0, &result);
  }, [&] (auto sched) {
    a = new double[n];
    taskparts::parallel_for(0, n, [&] (size_t i) {
      a[i] = 1.0;
    }, taskparts::dflt_parallel_for_cost_fn, sched);
  }, [&] (auto sched) {
    delete [] a;
  });
  
  printf("result=%f\n",result);
  return 0;
}
