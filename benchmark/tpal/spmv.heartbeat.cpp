#include <algorithm>
#include <cstdio>
#include <cstdint>
#include "rollforward.h"

#define D 64

void row_loop_spawn(
  float* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  float* x,
  float* y,
  uint64_t row_lo,
  uint64_t row_hi);
void col_loop_spawn(
  float* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  float* x,
  float* y,
  uint64_t row_lo,
  uint64_t row_hi,
  uint64_t col_lo,
  uint64_t col_hi,
  float t, uint64_t nb_rows);
void col_loop_col_loop_spawn(
  float* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  float* x,
  float* y,
  uint64_t col_lo,
  uint64_t col_hi,
  float t,
  float* dst);

void rollforward_handler_annotation __rf_handle_row_loop(
  float* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  float* x,
  float* y,
  uint64_t row_lo,
  uint64_t row_hi, bool& promoted) {
  
  if ((row_hi - row_lo) <= 1) {
    promoted = false;
  } else {
    row_loop_spawn(val, row_ptr, col_ind, x, y, row_lo, row_hi);    
    promoted = true;
  }
  rollbackward
}

void rollforward_handler_annotation __rf_handle_col_loop(
  float* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  float* x,
  float* y,
  uint64_t row_lo,
  uint64_t row_hi,
  uint64_t col_lo,
  uint64_t col_hi,
  float t, bool& promoted) {
  
  auto nb_rows = row_hi - row_lo;
  if (nb_rows == 0) {
    promoted = false;
  } else {
    promoted = true;
    col_loop_spawn(val, row_ptr, col_ind, x, y, row_lo, row_hi, col_lo, col_hi, t, nb_rows);
  }
 exit:
  rollbackward
}

void rollforward_handler_annotation __rf_handle_col_loop_col_loop(
  float* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  float* x,
  float* y,
  uint64_t col_lo,
  uint64_t col_hi,
  float t,
  float* dst, bool& promoted) {
  
  if ((col_hi - col_lo) <= 1) {
    promoted = false;
  } else {
    promoted = true;
    col_loop_col_loop_spawn(val, row_ptr, col_ind, x, y, col_lo, col_hi, t, dst);
  }
  rollbackward
}

#define D 1024

void spmv_interrupt(
  float* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  float* x,
  float* y,
  uint64_t row_lo,
  uint64_t row_hi) {
  if (! (row_lo < row_hi)) { // row loop
    return;
  }
  for (; ; ) { 
    float r = 0.0;
    uint64_t col_lo = row_ptr[row_lo];
    uint64_t col_hi = row_ptr[row_lo + 1];
    if (! (col_lo < col_hi)) { // col loop (1)
      goto exit_col_loop;
    }
    for (; ; ) {
      uint64_t col_lo2 = col_lo;
      uint64_t col_hi2 = std::min(col_lo + D, col_hi);
      for (; col_lo2 < col_hi2; col_lo2++) { // col loop (2)
        r += val[col_lo2] * x[col_ind[col_lo2]];
      }
      col_lo = col_lo2;
      if (! (col_lo < col_hi)) {
        break;
      }
      bool promoted = false;
      __rf_handle_col_loop(val, row_ptr, col_ind, x, y, row_lo, row_hi, col_lo, col_hi, r, promoted);
      if (rollforward_branch_unlikely(promoted)) {
	return;
      }
    }
  exit_col_loop:
    y[row_lo] = r;
    row_lo++;
    if (! (row_lo < row_hi)) {
      break;
    }
    bool promoted = false;
    __rf_handle_row_loop(val, row_ptr, col_ind, x, y, row_lo, row_hi, promoted);
    if (rollforward_branch_unlikely(promoted)) {
      return;
    }
  }
}

void spmv_interrupt_col_loop(
  float* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  float* x,
  float* y,
  uint64_t col_lo,
  uint64_t col_hi,
  float r,
  float* dst) {
  if (! (col_lo < col_hi)) {
    goto exit;
  }
  for (; ; ) {
    uint64_t col_lo2 = col_lo;
    uint64_t col_hi2 = std::min(col_lo + D, col_hi);
    for (; col_lo2 < col_hi2; col_lo2++) {
      r += val[col_lo2] * x[col_ind[col_lo2]];
    }
    col_lo = col_lo2;
    if (! (col_lo < col_hi)) {
      break;
    }
    bool promoted = false;
    __rf_handle_col_loop_col_loop(val, row_ptr, col_ind, x, y, col_lo, col_hi, r, dst, promoted);
    if (rollforward_branch_unlikely(promoted)) {
      return;
    }
  }
exit:
  *dst = r;
}

