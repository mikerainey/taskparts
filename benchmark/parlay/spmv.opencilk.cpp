#include "opencilk.hpp"
#include "spmv.hpp"
#include <cstdint>

int nworkers = -1;
auto ceiling_div(uint64_t x, uint64_t y) -> uint64_t {
  return (x + y - 1) / y;
}
auto get_grainsize(uint64_t n) -> uint64_t {
  return std::min<uint64_t>(2048, ceiling_div(n, 8 * nworkers));
}

auto dotprod_rec(int64_t lo, int64_t hi, float* val, float* x, uint64_t* col_ind,
		 uint64_t grainsize) -> float {
    auto n = hi - lo;
    if (n <= grainsize) {
      float t = 0.0;
      for (int64_t k = lo; k < hi; k++) { // col loop
	t += val[k] * x[col_ind[k]];
      }
      return t;
    }
    auto mid = (lo + hi) / 2;
    float r1, r2;
    r1 = cilk_spawn dotprod_rec(lo, mid, val, x, col_ind, grainsize);
    r2 = dotprod_rec(mid, hi, val, x, col_ind, grainsize);
    cilk_sync;
    return r1 + r2;
}

auto dotprod(int64_t lo, int64_t hi, float* val, float* x, uint64_t* col_ind) -> float {
  return dotprod_rec(lo, hi, val, x, col_ind, get_grainsize(hi - lo));
}

void spmv_cilk(float* val,
               uint64_t* row_ptr,
               uint64_t* col_ind,
               float* x,
               float* y,
               int64_t n) {
  /*
  cilk_for (int64_t i = 0; i < n; i++) {  // row loop
    opencilk::reducer_opadd<float> t(0.0);
    cilk_for (int64_t k = row_ptr[i]; k < row_ptr[i+1]; k++) { // col loop
      *t += val[k] * x[col_ind[k]];
    }
    y[i] = t.get_value();
  }
  */
  cilk_for (int64_t i = 0; i < n; i++) {  // row loop
    y[i] = dotprod(row_ptr[i], row_ptr[i+1], val, x, col_ind);
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
  nworkers = taskparts::perworker::nb_workers();
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
