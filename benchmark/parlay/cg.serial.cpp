#include "cg.hpp"

scalar dotp(uint64_t n, scalar* r, scalar* q) {
  scalar sum = 0.0;
  for (uint64_t j = 0; j < n; j++) {
    sum += r[j] * q[j];
  }
  return sum;
}

scalar norm(uint64_t n, scalar* r) {
  return dotp(n, r, r);
}

void spmv_serial(
  scalar* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  scalar* x,
  scalar* y,
  uint64_t n) {
  for (uint64_t i = 0; i < n; i++) { // row loop
    scalar r = 0.0;
#pragma clang loop vectorize_width(4) interleave_count(4)
    for (uint64_t k = row_ptr[i]; k < row_ptr[i + 1]; k++) { // col loop
      r += val[k] * x[col_ind[k]];
    }
    y[i] = r;
  }
}

scalar conj_grad (
    uint64_t n,
    uint64_t* col_ind,
    uint64_t* row_ptr,
    scalar* x,
    scalar* z,
    scalar* a,
    scalar* p,
    scalar* q,
    scalar* r,
    uint64_t cgitmax = 25) {
  for (uint64_t j = 0; j < n; j++) {
    q[j] = 0.0;
    z[j] = 0.0;
    r[j] = x[j];
    p[j] = r[j];
  }
  scalar rho = norm(n, r);
  for (uint64_t cgit = 0; cgit < cgitmax; cgit++) {
    scalar rho0 = rho;
    rho = 0.0;
    spmv_serial(a, row_ptr, col_ind, p, q, n);
    scalar alpha = rho0 / dotp(n, p, q);
    for (uint64_t j = 0; j < n; j++) {
      z[j] = z[j] + alpha * p[j];
      r[j] = r[j] - alpha * q[j];
      rho += r[j] * r[j];
    }
    scalar beta = rho / rho0;
    for (uint64_t j = 0; j < n; j++) {
      p[j] = r[j] + beta * p[j];
    }
  }
  spmv_serial(a, row_ptr, col_ind, z, r, n);
  scalar sum = 0.0;
  for (uint64_t j = 0; j < n; j++) {
    scalar d = x[j] - r[j];
    sum += d * d;
  }
  return sqrt(sum);
}


int main() {
  scalar rnorm;
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    rnorm = conj_grad(n, col_ind, row_ptr, x, y, val, p, q, r);
  }, [&] (auto sched) { bench_pre(); }, [&] (auto sched) { bench_post(); });
  return 0;
}
