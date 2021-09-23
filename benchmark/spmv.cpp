#include "taskparts/benchmark.hpp"
// -DNDEBUG -O3 -march=native -fno-verbose-asm -fno-stack-protector -fno-asynchronous-unwind-tables -fomit-frame-pointer -mavx2 -mfma -fopenmp

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
	while ((((h = taskparts::hash(h)) % 1000003) < 500001)) _pow += dim;
	j = (i + ((h = taskparts::hash(h)) % (((long) 1) << _pow))) % num_rows;
      } while (j == i);
    }
    edges.push_back(std::make_pair(i, j));
  }
  return edges;
}

char* random_bigrows_input = "random_bigrows";
char* random_bigcols_input = "random_bigcols";
char* arrowhead_input = "arrowhead";

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

#ifdef TASKPARTS_TPALRTS
extern
void spmv_serial(
  float* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  float* x,
  float* y,
  uint64_t n);
#else

void spmv_serial(
  float* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  float* x,
  float* y,
  uint64_t n) {
  for (uint64_t i = 0; i < n; i++) { // row loop
    float r = 0.0;
    #pragma omp simd reduction(+:r)
    for (uint64_t k = row_ptr[i]; k < row_ptr[i + 1]; k++) { // col loop
      r += val[k] * x[col_ind[k]];
    }
    y[i] = r;
  }
}
#endif

int main() {
  nb_rows = n_bigrows;
  degree = degree_bigrows;
  auto edges = mk_random_local_edgelist(dim, degree, nb_rows);
  csr_of_edgelist(edges);
  
  x = (float*)malloc(sizeof(float) * nb_rows);
  y = (float*)malloc(sizeof(float) * nb_rows);
  if ((val == nullptr) || (row_ptr == nullptr) || (col_ind == nullptr) || (x == nullptr) || (y == nullptr)) {
    printf("oops\n");
    exit(1);
  }
  // initialize x and y vectors
  {
    for (size_t i = 0; i < nb_rows; i++) {
      x[i] = rand_float(i);
    }
    zero_init(y, nb_rows);
  }
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    spmv_serial(val, row_ptr, col_ind, x, y, nb_rows);
  });
  return 0;
}
