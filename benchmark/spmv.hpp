#pragma once

#include <taskparts/benchmark.hpp>
// -DNDEBUG -O3 -march=native -fno-verbose-asm -fno-stack-protector -fno-asynchronous-unwind-tables -fomit-frame-pointer -mavx2 -mfma -fno-verbose-asm

auto rand_float(size_t i) -> float {
  int m = 1000000;
  float v = taskparts::hash(i) % m;
  return v / m;
}

using edge_type = std::pair<uint64_t, uint64_t>;

using edgelist_type = std::vector<edge_type>;

auto mk_random_local_edgelist(size_t dim, size_t degree, size_t num_rows)
  -> edgelist_type {
  size_t non_zeros = num_rows*degree;
  edgelist_type edges;
  for (size_t k = 0; k < non_zeros; k++) { 
    size_t i = k / degree;
    size_t j;
    if (dim==0) {
      size_t h = k;
      do {
	j = ((h = taskparts::hash(h)) % num_rows);
      } while (j == i);
    } else {
      size_t _pow = dim+2;
      size_t h = k;
      do {
	while ((((h = taskparts::hash(h)) % 100003) < 500001)) _pow += dim;
	j = (i + ((h = taskparts::hash(h)) % (((long) 1) << _pow))) % num_rows;
      } while (j == i);
    }
    edges.push_back(std::make_pair(i, j));
  }
  return edges;
}

auto mk_powerlaw_edgelist(size_t lg) {
  edgelist_type edges;
  size_t o = 0;
  size_t m = 1 << lg;
  size_t n = 1;
  size_t tot = m;
  for (size_t i = 0; i < lg; i++) {
    for (size_t j = 0; j < n; j++) {
      for (size_t k = 0; k < m; k++) {
	auto d = taskparts::hash(o + k) % tot;
	edges.push_back(std::make_pair(o, d));
      }
      o++;
    }
    n *= 2;
    m /= 2;
  }
  
  return edges; 
}

auto mk_arrowhead_edgelist(size_t n) {
  edgelist_type edges;
  for (size_t i = 0; i < n; i++) {
    edges.push_back(std::make_pair(i, 0));
  }
  for (size_t i = 0; i < n; i++) {
    edges.push_back(std::make_pair(0, i));
  }
  for (size_t i = 0; i < n; i++) {
    edges.push_back(std::make_pair(i, i));
  }
  return edges;
}

const char* random_bigrows_input = "random_bigrows";
const char* random_bigcols_input = "random_bigcols";
const char* arrowhead_input = "arrowhead";

uint64_t n_bigrows = 300000;
uint64_t degree_bigrows = 100;
uint64_t n_bigcols = 23;
uint64_t n_arrowhead = 100000000;
  
uint64_t row_len = 1000;
uint64_t degree = 4;
uint64_t dim = 5;
uint64_t nb_rows;
uint64_t nb_vals;
float* val;
uint64_t* row_ptr;
uint64_t* col_ind;
float* x;
float* y;

auto csr_of_edgelist(edgelist_type& edges) {
  std::sort(edges.begin(), edges.end(), std::less<edge_type>());
  {
    edgelist_type edges2;
    edge_type prev = edges.back();
    for (auto& e : edges) {
      if (e != prev) {
	edges2.push_back(e);
      }
      prev = e;
    }
    edges2.swap(edges);
    edges2.clear();
  }
  nb_vals = edges.size();
  row_ptr = (uint64_t*)malloc(sizeof(uint64_t) * (nb_rows + 1));
  for (size_t i = 0; i < nb_rows + 1; i++) {
    row_ptr[i] = 0;
  }
  for (auto& e : edges) {
    assert((e.first >= 0) && (e.first < nb_rows));
    row_ptr[e.first]++;
  }
  size_t max_col_len = 0;
  size_t tot_col_len = 0;
  {
    for (size_t i = 0; i < nb_rows + 1; i++) {
      max_col_len = std::max(max_col_len, row_ptr[i]);
      tot_col_len += row_ptr[i];
    }
  }
  { // initialize column indices
    col_ind = (uint64_t*)malloc(sizeof(uint64_t) * nb_vals);
    uint64_t i = 0;
    for (auto& e : edges) {
      col_ind[i++] = e.second;
    }
  }
  edges.clear();
  { // initialize row pointers
    uint64_t a = 0;
    for (size_t i = 0; i < nb_rows; i++) {
      auto e = row_ptr[i];
      row_ptr[i] = a;
      a += e;
    }
    row_ptr[nb_rows] = a;
  }
  { // initialize nonzero values array
    val = (float*)malloc(sizeof(float) * nb_vals);
    for (size_t i = 0; i < nb_vals; i++) {
      val[i] = rand_float(i);
    }
  }
  printf("nb_vals %lu\n", nb_vals);
  printf("max_col_len %lu\n", max_col_len);
  printf("tot_col_len %lu\n", tot_col_len);
}

template <typename T>
void zero_init(T* a, std::size_t n) {
  volatile T* b = (volatile T*)a;
  for (std::size_t i = 0; i < n; i++) {
    b[i] = 0;
  }
}

template <typename Gen_matrix>
auto bench_pre_shared(const Gen_matrix& gen_matrix) {
  gen_matrix();
  x = (float*)malloc(sizeof(float) * nb_rows);
  y = (float*)malloc(sizeof(float) * nb_rows);
  if ((val == nullptr) || (row_ptr == nullptr) || (col_ind == nullptr) || (x == nullptr) || (y == nullptr)) {
    exit(1);
  }
  // initialize x and y vectors
  {
    for (size_t i = 0; i < nb_rows; i++) {
      x[i] = rand_float(i);
    }
    zero_init(y, nb_rows);
  }  
};

auto bench_pre_bigrows() {
  nb_rows = n_bigrows;
  degree = degree_bigrows;
  bench_pre_shared([&] {
    auto edges = mk_random_local_edgelist(dim, degree, nb_rows);
    csr_of_edgelist(edges);
  });
}

auto bench_pre_bigcols() {
  nb_rows = 1<<n_bigcols;
  bench_pre_shared([&] {
    auto edges = mk_powerlaw_edgelist(n_bigcols);
    csr_of_edgelist(edges);
  });
}

auto bench_pre_arrowhead() {
  nb_rows = n_arrowhead;
  bench_pre_shared([&] {
    auto edges = mk_arrowhead_edgelist(nb_rows);
    csr_of_edgelist(edges);
  });
}

extern
void spmv_serial(
  float* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  float* x,
  float* y,
  uint64_t n);

/*
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
*/

extern
void spmv_interrupt(
  float* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  float* x,
  float* y,
  uint64_t row_lo,
  uint64_t row_hi);

extern
void spmv_interrupt_col_loop(
  float* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  float* x,
  float* y,
  uint64_t col_lo,
  uint64_t col_hi,
  float t,
  float* dst);

auto bench_pre() {
  taskparts::cmdline::dispatcher d;
  d.add("bigcols", bench_pre_bigcols);
  d.add("bigrows", bench_pre_bigrows);
  d.add("arrowhead", bench_pre_arrowhead);
  d.dispatch_or_default("input_matrix", "bigcols");
}

auto bench_post() {
#ifndef NDEBUG
  double* yref = (double*)malloc(sizeof(double) * nb_rows);
  {
    for (uint64_t i = 0; i != nb_rows; i++) {
      yref[i] = 1.0;
    }
    spmv_serial(val, row_ptr, col_ind, x, yref, nb_rows);
  }
  uint64_t nb_diffs = 0;
  double epsilon = 0.01;
  for (uint64_t i = 0; i != nb_rows; i++) {
    auto diff = std::abs(y[i] - yref[i]);
    if (diff > epsilon) {
      //printf("diff=%f y[i]=%f yref[i]=%f at i=%ld\n", diff, y[i], yref[i], i);
      nb_diffs++;
    }
  }
  //aprintf("nb_diffs %ld\n", nb_diffs);
  free(yref);
#endif
  free(val);
  free(row_ptr);
  free(col_ind);
  free(x);
  free(y);
};
