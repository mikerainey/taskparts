#include "cg.hpp"
#include "opencilk.hpp"
#include <cstdint>

int nworkers = -1;
auto ceiling_div(uint64_t x, uint64_t y) -> uint64_t {
  return (x + y - 1) / y;
}
auto get_grainsize(uint64_t n) -> uint64_t {
  return std::min<uint64_t>(2048, ceiling_div(n, 8 * nworkers));
}

auto sparse_dotp_rec(int64_t lo, int64_t hi, scalar* val, scalar* x, uint64_t* col_ind,
		     uint64_t grainsize) -> scalar {
  auto n = hi - lo;
  if (n <= grainsize) {
    scalar t = 0.0;
    for (int64_t k = lo; k < hi; k++) { // col loop
      t += val[k] * x[col_ind[k]];
    }
    return t;
  }
  auto mid = (lo + hi) / 2;
  scalar r1, r2;
  r1 = cilk_spawn sparse_dotp_rec(lo, mid, val, x, col_ind, grainsize);
  r2 = sparse_dotp_rec(mid, hi, val, x, col_ind, grainsize);
  cilk_sync;
  return r1 + r2;
}

auto sparse_dotp(int64_t lo, int64_t hi, scalar* val, scalar* x, uint64_t* col_ind) -> scalar {
  return sparse_dotp_rec(lo, hi, val, x, col_ind, get_grainsize(hi - lo));
}

void spmv_cilk(scalar* val,
               uint64_t* row_ptr,
               uint64_t* col_ind,
               scalar* x,
               scalar* y,
               int64_t n) {
  cilk_for (int64_t i = 0; i < n; i++) {  // row loop
    y[i] = sparse_dotp(row_ptr[i], row_ptr[i+1], val, x, col_ind);
  }
}

auto dotp_rec(int64_t lo, int64_t hi, scalar* r, scalar* q, uint64_t grainsize) -> scalar {
  auto n = hi - lo;
  if (n <= grainsize) {
    scalar sum = 0.0;
    for (uint64_t j = lo; j < hi; j++) {
      sum += r[j] * q[j];
    }
    return sum;
  }
  auto mid = (lo + hi) / 2;
  scalar r1, r2;
  r1 = cilk_spawn dotp_rec(lo, mid, r, q, grainsize);
  r2 = dotp_rec(mid, hi, r, q, grainsize);
  cilk_sync;
  return r1 + r2;
}

scalar dotp(uint64_t n, scalar* r, scalar* q) {
  return dotp_rec(0, n, r, q, get_grainsize(n));
}

scalar norm(uint64_t n, scalar* r) {
  return dotp(n, r, r);
}

auto loop1_rec(int64_t lo, int64_t hi, scalar* z, scalar* p, scalar* q, scalar* r, scalar alpha, uint64_t grainsize) -> scalar {
  auto n = hi - lo;
  if (n <= grainsize) {
    scalar sum = 0.0;
    for (uint64_t j = 0; j < n; j++) {
      z[j] = z[j] + alpha * p[j];
      r[j] = r[j] - alpha * q[j];
      sum += r[j] * r[j];
    }
    return sum;
  }
  auto mid = (lo + hi) / 2;
  scalar r1, r2;
  r1 = cilk_spawn loop1_rec(lo, mid, z, p, q, r, alpha, grainsize);
  r2 = loop1_rec(mid, hi, z, p, q, r, alpha, grainsize);
  cilk_sync;
  return r1 + r2;
}

auto loop1(int64_t n, scalar* z, scalar* p, scalar* q, scalar* r, scalar alpha) -> scalar {
  return loop1_rec(0, n, z, p, q, r, alpha, get_grainsize(n));
}
  
auto loop2_rec(int64_t lo, int64_t hi, scalar* x, scalar* r, uint64_t grainsize) -> scalar {
  auto n = hi - lo;
  if (n <= grainsize) {
    scalar sum = 0.0;
    for (uint64_t j = lo; j < hi; j++) {
      scalar d = x[j] - r[j];
      sum += d * d;
    }
    return sum;
  }
  auto mid = (lo + hi) / 2;
  scalar r1, r2;
  r1 = cilk_spawn loop2_rec(lo, mid, x, r, grainsize);
  r2 = loop2_rec(mid, hi, x, r, grainsize);
  cilk_sync;
  return r1 + r2;
}

auto loop2(uint64_t n, scalar* x, scalar* r) -> scalar {
  return loop2_rec(0, n, x, r, get_grainsize(n));
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
  cilk_for (uint64_t j = 0; j < n; j++) {
    q[j] = 0.0;
    z[j] = 0.0;
    r[j] = x[j];
    p[j] = r[j];
  }
  scalar rho = norm(n, r);
  for (uint64_t cgit = 0; cgit < cgitmax; cgit++) {
    scalar rho0 = rho;
    rho = 0.0;
    spmv_cilk(a, row_ptr, col_ind, p, q, n);
    scalar alpha = rho0 / dotp(n, p, q);
    rho += loop1(n, z, p, q, r, alpha);
    scalar beta = rho / rho0;
    cilk_for (uint64_t j = 0; j < n; j++) {
      p[j] = r[j] + beta * p[j];
    }
  }
  spmv_cilk(a, row_ptr, col_ind, z, r, n);
  return sqrt(loop2(n, x, r));
}


int main() {
  scalar rnorm;
  nworkers = taskparts::perworker::nb_workers();
  taskparts::benchmark_cilk([&] {
    rnorm = conj_grad(n, col_ind, row_ptr, x, y, val, p, q, r);
  }, [&] { bench_pre(); }, [&] { bench_post(); });
  return 0;
}
