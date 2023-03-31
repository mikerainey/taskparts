#ifndef TASKPARTS_TPALRTS
#error "need to compile with tpal flags, e.g., TASKPARTS_TPALRTS"
#endif
// TODO: move spmv.hpp to this folder and introduce loader for the matrix market file format:
// https://github.com/cwpearson/matrix-market
#include "../parlay/spmv.hpp"
#define SPMV_INCLUDE
#include "../parlay/spmv.serial.hpp"
#include <cstdint>

extern
void spmv_interrupt(
  nonzero_type* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  nonzero_type* x,
  nonzero_type* y,
  uint64_t row_lo,
  uint64_t row_hi);
extern
void spmv_interrupt_col_loop(
  nonzero_type* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  nonzero_type* x,
  nonzero_type* y,
  uint64_t col_lo,
  uint64_t col_hi,
  nonzero_type r,
  nonzero_type* dst);

// later: if we inline the function below to its callsite, the rollforwad compiler may erroneously
// treat as handler calls the lambda functions in the fork/join instruction, which seems to be
// because the compiler generates a name for the lambda functions that contains the magic prefix
// __rf_handle...
void row_loop_spawn(
  nonzero_type* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  nonzero_type* x,
  nonzero_type* y,
  uint64_t row_lo,
  uint64_t row_hi) {
    auto mid = (row_lo + row_hi) / 2;
    tpalrts_promote_via_nativefj([=] {
      spmv_interrupt(val, row_ptr, col_ind, x, y, row_lo, mid);
    }, [=] {
      spmv_interrupt(val, row_ptr, col_ind, x, y, mid, row_hi);
    }, [=] { }, taskparts::bench_scheduler());
}

void col_loop_spawn(
  nonzero_type* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  nonzero_type* x,
  nonzero_type* y,
  uint64_t row_lo,
  uint64_t row_hi,
  uint64_t col_lo,
  uint64_t col_hi,
  nonzero_type t, uint64_t nb_rows) {
    auto cf = [=] {
      spmv_interrupt_col_loop(val, row_ptr, col_ind, x, y, col_lo, col_hi, t, &y[row_lo]);
    };
    if (nb_rows == 1) {
      cf();
      return;
    }
    row_lo++;
    if (nb_rows == 2) {
      tpalrts_promote_via_nativefj(cf, [=] {
	spmv_interrupt(val, row_ptr, col_ind, x, y, row_lo, row_hi);
      }, [=] { }, taskparts::bench_scheduler());
    } else {
      auto row_mid = (row_lo + row_hi) / 2;
      tpalrts_promote_via_nativefj([=] {
	spmv_interrupt(val, row_ptr, col_ind, x, y, row_lo, row_mid);
      }, [=] {
	spmv_interrupt(val, row_ptr, col_ind, x, y, row_mid, row_hi);
      }, [=] { }, taskparts::bench_scheduler());
    }
}

void col_loop_col_loop_spawn(
  nonzero_type* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  nonzero_type* x,
  nonzero_type* y,
  uint64_t col_lo,
  uint64_t col_hi,
  nonzero_type t,
  nonzero_type* dst) {
    auto col_mid = (col_lo + col_hi) / 2;
    std::pair<nonzero_type, nonzero_type> fdst;
    auto dstp = &fdst;
    tpalrts_promote_via_nativefj([&] {
      spmv_interrupt_col_loop(val, row_ptr, col_ind, x, y, col_lo, col_mid, t, &(dstp->first));
    }, [&] {
      spmv_interrupt_col_loop(val, row_ptr, col_ind, x, y, col_mid, col_hi, 0.0, &(dstp->second));
    }, [&] { 
      *dst = dstp->first + dstp->second;
    }, taskparts::bench_scheduler());
}

int main() {
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    spmv_interrupt(val, row_ptr, col_ind, x, y, 0, nb_rows);
  }, [&] (auto sched) {
    bench_pre();
  }, [&] (auto sched) {
    bench_post();
  });
  
  return 0;
}

