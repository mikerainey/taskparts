/*
// -DNDEBUG -O3 -march=native -fno-verbose-asm -fno-stack-protector -fno-asynchronous-unwind-tables -fomit-frame-pointer -mavx2 -mfma -fno-verbose-asm
./gen_rollforward.sh spmv spmv
 */


#include <stdint.h>
#include <algorithm>

#define unlikely(x)    __builtin_expect(!!(x), 0)

#define D 1024

extern volatile 
int heartbeat;

extern
int row_loop_handler(
  float* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  float* x,
  float* y,
  uint64_t row_lo,
  uint64_t row_hi);

extern
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
  float r);

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
#pragma clang loop vectorize_width(4) interleave_count(4)
      for (; col_lo2 < col_hi2; col_lo2++) { // col loop (2)
        r += val[col_lo2] * x[col_ind[col_lo2]];
      }
      col_lo = col_lo2;
      if (! (col_lo < col_hi)) {
        break;
      }
      if (unlikely(heartbeat)) {
        if (col_loop_handler(val, row_ptr, col_ind, x, y, row_lo, row_hi, col_lo, col_hi, r)) {
          return;
        }
      }
    }
  exit_col_loop:
    y[row_lo] = r;
    row_lo++;
    if (! (row_lo < row_hi)) {
      break;
    }
    if (unlikely(heartbeat)) {
      if (row_loop_handler(val, row_ptr, col_ind, x, y, row_lo, row_hi)) {
        return;
      }
    }
  }
}

extern
int col_loop_handler_col_loop(
  float* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  float* x,
  float* y,
  uint64_t col_lo,
  uint64_t col_hi,
  float r,
  float* dst);

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
#pragma clang loop vectorize_width(4) interleave_count(4)
    for (; col_lo2 < col_hi2; col_lo2++) {
      r += val[col_lo2] * x[col_ind[col_lo2]];
    }
    col_lo = col_lo2;
    if (! (col_lo < col_hi)) {
      break;
    }
    if (unlikely(heartbeat)) {
      if (col_loop_handler_col_loop(val, row_ptr, col_ind, x, y, col_lo, col_hi, r, dst)) {
        return;
      }
    }
  }
exit:
  *dst = r;
}

