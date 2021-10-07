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
  return 0;
  /*
  auto cf = [=] (tpalrts::promotable* p2) {
    spmv_interrupt_col_loop(val, row_ptr, col_ind, x, y, col_lo, col_hi, t, &y[row_lo], p2);
  };
  if (nb_rows == 1) {
    p->async_finish_promote(cf);
    return 1;
  }
  row_lo++;
  if (nb_rows == 2) {
    p->fork_join_promote2(cf, [=] (tpalrts::promotable* p2) {
      spmv_interrupt(val, row_ptr, col_ind, x, y, row_lo, row_hi, p2);
    }, [=] (tpalrts::promotable*) {
      // nothing left to do
    });
  } else {
    auto row_mid = (row_lo + row_hi) / 2;
    p->fork_join_promote3(cf, [=] (tpalrts::promotable* p2) {
      spmv_interrupt(val, row_ptr, col_ind, x, y, row_lo, row_mid, p2);
    }, [=] (tpalrts::promotable* p2) {
      spmv_interrupt(val, row_ptr, col_ind, x, y, row_mid, row_hi, p2);
    }, [=] (tpalrts::promotable*) {
      // nothing left to do
    });    
  }
  return 1; */
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
  return 0;
  /*
  auto col_mid = (col_lo + col_hi) / 2;
  using dst_rec_type = std::pair<float, float>;
  dst_rec_type* dst_rec;
  tpalrts::arena_block_type* dst_blk;
  std::tie(dst_rec, dst_blk) = tpalrts::alloc_arena<dst_rec_type>();
  p->fork_join_promote2([=] (tpalrts::promotable* p2) {
    spmv_interrupt_col_loop(val, row_ptr, col_ind, x, y, col_lo, col_mid, t, &(dst_rec->first), p2);
  }, [=] (tpalrts::promotable* p2) {
    spmv_interrupt_col_loop(val, row_ptr, col_ind, x, y, col_mid, col_hi, 0.0, &(dst_rec->second), p2);
  }, [=] (tpalrts::promotable*) {
    *dst = dst_rec->first + dst_rec->second;
    decr_arena_block(dst_blk);
  });
  return 1; */
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
  bench_pre_bigcols();
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    spmv_interrupt(val, row_ptr, col_ind, x, y, 0, nb_rows);
  });
  return 0;
}
