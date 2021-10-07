#include "cilk.hpp"
#include "spmv.hpp"

void spmv_cilk(float* val,
               uint64_t* row_ptr,
               uint64_t* col_ind,
               float* x,
               float* y,
               int64_t n) {
  cilk_for (int64_t i = 0; i < n; i++) {  // row loop
    cilk::reducer_opadd<float> t(0.0);
    cilk_for (int64_t k = row_ptr[i]; k < row_ptr[i+1]; k++) { // col loop
      *t += val[k] * x[col_ind[k]];
    }
    y[i] = t.get_value();
  }
}

void spmv_cilk_outer(float* val,
		     uint64_t* row_ptr,
		     uint64_t* col_ind,
		     float* x,
		     float* y,
		     int64_t n) {
  cilk_for (int64_t i = 0; i < n; i++) {  // row loop
    float t = 0.0;
    for (int64_t k = row_ptr[i]; k < row_ptr[i+1]; k++) { // col loop
      t += val[k] * x[col_ind[k]];
    }
    y[i] = t;
  }
}

int main() {
  taskparts::benchmark_cilk([&] {
      taskparts::cmdline::dispatcher d;
      d.add("default", [&] {
	spmv_cilk(val, row_ptr, col_ind, x, y, nb_rows);
      });
      d.add("outer", [&] {
	spmv_cilk_outer(val, row_ptr, col_ind, x, y, nb_rows);
      });
      d.dispatch_or_default("variant", "default");
  }, [&] { bench_pre(); }, [&] { bench_post(); });
  return 0;
}
