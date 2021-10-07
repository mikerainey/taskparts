#ifndef TASKPARTS_TPALRTS
#error "need to compile with tpal flags, e.g., TASKPARTS_TPALRTS"
#endif
#include "spmv.hpp"
#include "spmv_rollforward_decls.hpp"

int row_loop_handler(
  float* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  float* x,
  float* y,
  uint64_t row_lo,
  uint64_t row_hi) {
  if ((row_hi - row_lo) <= 1) {
    return 0;
  }
  auto mid = (row_lo + row_hi) / 2;
  taskparts::tpalrts_promote_via_nativefj([=] {
    spmv_interrupt(val, row_ptr, col_ind, x, y, row_lo, mid);
  }, [=] {
    spmv_interrupt(val, row_ptr, col_ind, x, y, mid, row_hi);
  }, [=] { }, taskparts::bench_scheduler());
  return 1;
}

int col_loop_handler(
  float* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  float* x,
  float* y,
  uint64_t row_lo,
  uint64_t row_hi,
  uint64_t col_lo,
  uint64_t col_hi,
  float t) {
  auto nb_rows = row_hi - row_lo;
  if (nb_rows == 0) {
    return 0;
  }
  auto cf = [=] {
    spmv_interrupt_col_loop(val, row_ptr, col_ind, x, y, col_lo, col_hi, t, &y[row_lo]);
  };
  if (nb_rows == 1) {
    cf();
    return 1;
  }
  row_lo++;
  if (nb_rows == 2) {
    taskparts::tpalrts_promote_via_nativefj(cf, [=] {
      spmv_interrupt(val, row_ptr, col_ind, x, y, row_lo, row_hi);
    }, [=] { }, taskparts::bench_scheduler());
  } else {
    auto row_mid = (row_lo + row_hi) / 2;
    taskparts::tpalrts_promote_via_nativefj([=] {
      cf();
      spmv_interrupt(val, row_ptr, col_ind, x, y, row_lo, row_mid);
    }, [=] {
      spmv_interrupt(val, row_ptr, col_ind, x, y, row_mid, row_hi);
    }, [=] { }, taskparts::bench_scheduler());
  }
  return 1;
}

int col_loop_handler_col_loop(
  float* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  float* x,
  float* y,
  uint64_t col_lo,
  uint64_t col_hi,
  float t,
  float* dst) {
  if ((col_hi - col_lo) <= 1) {
    return 0;
  }
  auto col_mid = (col_lo + col_hi) / 2;
  float dst1, dst2;
  taskparts::tpalrts_promote_via_nativefj([&] {
    spmv_interrupt_col_loop(val, row_ptr, col_ind, x, y, col_lo, col_mid, t, &dst1);
  }, [&] {
    spmv_interrupt_col_loop(val, row_ptr, col_ind, x, y, col_mid, col_hi, 0.0, &dst2);
  }, [&] {
    *dst = dst1 + dst2;
  }, taskparts::bench_scheduler());
  return 1;
}

namespace taskparts {
auto initialize_rollforward() {
  rollforward_table = {
    #include "spmv_rollforward_map.hpp"
  };
  initialize_rollfoward_table();
}
} // end namespace

int main() {
  taskparts::initialize_rollforward();
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    spmv_interrupt(val, row_ptr, col_ind, x, y, 0, nb_rows);
  }, [&] (auto sched) { bench_pre(); }, [&] (auto sched) { bench_post(); });
  return 0;
}
