#include "taskparts/fiber.hpp"
#include "taskparts/nativeforkjoin.hpp"
#include <optional>
#ifndef TASKPARTS_TPALRTS
#error "need to compile with tpal flags, e.g., TASKPARTS_TPALRTS"
#endif
#include <taskparts/benchmark.hpp>

using namespace taskparts;

void sum_array_heartbeat(double* a, uint64_t lo, uint64_t hi, double r, double* dst);

/* Handler functions */
/* ================= */

void sum_array_handler(double *a, uint64_t lo, uint64_t hi, double r, double *dst,
		       future*& fut) {
  assert(fut == nullptr);
  if (lo == hi) {
    // nothing to promote
  } else if ((lo + 1) == hi) {
    return;
    /*
    auto fut0 = new lazy_future([=] {
      // todo: fix dst
      sum_array_heartbeat(a, lo + 1, hi, 0.0, dst);
    }, bench_scheduler());
    fut0->release();
    yield(bench_scheduler());
    fut = fut0; */
  } else {
    lo++;
    assert(lo < hi);
    fut = new lazy_future([=] {
      sum_array_heartbeat(a, lo, hi, 0.0, dst);
      /*
      double dst1, dst2;
      auto mid = (lo + hi) / 2;
      // todo fix dst
      fork2join([&] {
	sum_array_heartbeat(a, lo, mid, 0.0, &dst1);
      }, [&] {
	sum_array_heartbeat(a, mid, hi, 0.0, &dst2);
      }, bench_scheduler());
      *dst = r + dst1 + dst2; */
    }, bench_scheduler());
  }
}

void taskparts_tpal_handler __rf_handle_sum_array_heartbeat(double* a, uint64_t lo, uint64_t hi,
							    double r, double* dst,
							    future*& fut) {
  sum_array_handler(a, lo, hi, r, dst, fut);
  taskparts_tpal_rollbackward
}

/* Outlined-loop functions */
/* ======================= */

#define D 64

void sum_array_heartbeat(double* a, uint64_t lo, uint64_t hi, double r, double* dst) {
  future* fut = nullptr;
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
    assert(fut == nullptr);
    __rf_handle_sum_array_heartbeat(a, lo, hi, r, dst, fut);
    if (taskparts_tpal_unlikely(fut != nullptr)) {
      hi = lo + 1;
      auto fut0 = (fiber<bench_scheduler>*)fut;
      fut0->release();
      yield(bench_scheduler());
    }
  }
  if (fut != nullptr) {
    TASKPARTS_LOG_PPT(bench_scheduler, (void*)lo);
    fut->force();
    TASKPARTS_LOG_PPT(bench_scheduler, fut);
  }
 exit:
  *dst = r;
}

int main() {
  uint64_t n = cmdline::parse_or_default_long("n", 1000 * 1000 * 1000);
  double result = 0.0;
  double* a;
  benchmark_nativeforkjoin([&] (auto sched) {
    sum_array_heartbeat(a, 0, n, 0.0, &result);
  }, [&] (auto sched) {
    a = new double[n];
    parallel_for(0, n, [&] (size_t i) {
      a[i] = 1.0;
    }, dflt_parallel_for_cost_fn, sched);
  }, [&] (auto sched) {
    delete [] a;
  });
  
  printf("result=%f\n",result);
  return 0;
}
