#pragma once

#include <cstdint>
#include <stdio.h>
#include <stdlib.h>
#include "mm.hpp" // matrix market loader: https://github.com/cwpearson/matrix-market

#ifndef USE_FLOAT
using scalar = double;
#else
using scalar = float;
#endif

#include <taskparts/benchmark.hpp>

uint64_t n;
uint64_t nb_vals;
scalar* val;
uint64_t* row_ptr;
uint64_t* col_ind;
scalar* x;
scalar* y;
scalar *p;
scalar *q;
scalar* r;

auto rand_float(size_t i) -> scalar {
  int m = 1000000;
  scalar v = taskparts::hash(i) % m;
  return v / m;
}

template <typename T>
void zero_init(T* a, std::size_t n) {
  volatile T* b = (volatile T*)a;
  for (std::size_t i = 0; i < n; i++) {
    b[i] = 0;
  }
}

auto bench_pre_matrix_market() -> void {
  std::string fname = taskparts::cmdline::parse_or_default_string("fname", "1138_bus.mtx");
  typedef size_t Offset;
  typedef uint64_t Ordinal;
  typedef scalar Scalar;
  typedef MtxReader<Ordinal, Scalar> reader_t;
  typedef typename reader_t::coo_type coo_t;
  typedef typename coo_t::entry_type entry_t;
  typedef CSR<Ordinal, Scalar, Offset> csr_type;
  
  // read matrix as coo
  reader_t reader(fname);
  coo_t coo = reader.read_coo();
  csr_type csr(coo);
  nb_vals = csr.nnz();
  val = (scalar*)malloc(sizeof(scalar) * nb_vals);
  std::copy(csr.val().begin(), csr.val().end(), val);
  auto nb_rows = csr.num_rows();
  auto nb_cols = csr.num_cols();
  if (nb_rows != nb_cols) {  // Houson, we have a problem...
    taskparts::die("input matrix must be *symmetric* positive definite");
  }
  n = nb_rows;
  row_ptr = (Ordinal*)malloc(sizeof(Ordinal) * (n + 1));
  std::copy(csr.row_ptr().begin(), csr.row_ptr().end(), row_ptr);
  col_ind = (Offset*)malloc(sizeof(Offset) * nb_vals);
  std::copy(&csr.col_ind()[0], (&csr.col_ind()[nb_cols-1]) + 1, col_ind);

  x = (scalar*)malloc(sizeof(scalar) * n);
  y = (scalar*)malloc(sizeof(scalar) * n);
  p = (scalar*)malloc(sizeof(scalar) * n);
  q = (scalar*)malloc(sizeof(scalar) * n);
  r = (scalar*)malloc(sizeof(scalar) * n);

  if ((val == nullptr) || (row_ptr == nullptr) || (col_ind == nullptr) || (x == nullptr) || (y == nullptr)) {
    exit(1);
  }
  // initialize x and y vectors
  {
    for (size_t i = 0; i < n; i++) {
      x[i] = rand_float(i);
    }
    zero_init(y, n);
    zero_init(p, n);
    zero_init(q, n);
    zero_init(r, n);
  }  

  printf("nbr=%lu nnz=%lu\n",n,nb_vals);
}

auto bench_pre() -> void {
  bench_pre_matrix_market();
}

auto bench_post() {
  free(val);
  free(row_ptr);
  free(col_ind);
  free(x);
  free(y);
  free(p);
  free(q);
  free(r);
}
