#include "spmv.hpp"

void spmv_serial(
  float* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  float* x,
  float* y,
  uint64_t n) {
  for (uint64_t i = 0; i < n; i++) { // row loop
    float r = 0.0;
#pragma clang loop vectorize_width(4) interleave_count(4)
    for (uint64_t k = row_ptr[i]; k < row_ptr[i + 1]; k++) { // col loop
      r += val[k] * x[col_ind[k]];
    }
    y[i] = r;
  }
}

int main() {
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    spmv_serial(val, row_ptr, col_ind, x, y, nb_rows);
  }, [&] (auto sched) { bench_pre(); }, [&] (auto sched) { bench_post(); });
  return 0;
}
