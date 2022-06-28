#ifndef TASKPARTS_TPALRTS
#error "need to compile with tpal flags, e.g., TASKPARTS_TPALRTS"
#endif
#include <taskparts/benchmark.hpp>
#include <cstdint>
#if defined(TASKPARTS_LINUX)
#include <cmath>
double mypow(double x, double y) {
  return std::pow(x, y);
}
#elif defined(TASKPARTS_NAUTILUS)
extern "C"
double pow(double x, double y);
double mypow(double x, double y) {
  return pow(x, y);
}
#endif

void spmv_interrupt(
  double* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  double* x,
  double* y,
  uint64_t row_lo,
  uint64_t row_hi);
void spmv_interrupt_col_loop(
  double* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  double* x,
  double* y,
  uint64_t col_lo,
  uint64_t col_hi,
  double r,
  double* dst);


/* Handler functions */
/* ================= */

// later: if we inline the function below to its callsite, the rollforwad compiler may erroneously
// treat as handler calls the lambda functions in the fork/join instruction, which seems to be
// because the compiler generates a name for the lambda functions that contains the magic prefix
// __rf_handle...
void row_loop_spawn(
  double* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  double* x,
  double* y,
  uint64_t row_lo,
  uint64_t row_hi) {
    auto mid = (row_lo + row_hi) / 2;
    tpalrts_promote_via_nativefj([=] {
      spmv_interrupt(val, row_ptr, col_ind, x, y, row_lo, mid);
    }, [=] {
      spmv_interrupt(val, row_ptr, col_ind, x, y, mid, row_hi);
    }, [=] { }, taskparts::bench_scheduler());
}

void taskparts_tpal_handler __rf_handle_row_loop(
  double* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  double* x,
  double* y,
  uint64_t row_lo,
  uint64_t row_hi, bool& promoted) {
  
  if ((row_hi - row_lo) <= 1) {
    promoted = false;
  } else {
    row_loop_spawn(val, row_ptr, col_ind, x, y, row_lo, row_hi);    
    promoted = true;
  }
  taskparts_tpal_rollbackward
}

void col_loop_spawn(
  double* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  double* x,
  double* y,
  uint64_t row_lo,
  uint64_t row_hi,
  uint64_t col_lo,
  uint64_t col_hi,
  double t, uint64_t nb_rows) {
    auto cf = [=] {
      spmv_interrupt_col_loop(val, row_ptr, col_ind, x, y, col_lo, col_hi, t, &y[row_lo]);
    };
    if (nb_rows == 1) {
      cf();
      return;
    }
    row_lo++;
    if (nb_rows == 2) {
      tpalrts_promote_via_nativefj(cf, [=] {
	spmv_interrupt(val, row_ptr, col_ind, x, y, row_lo, row_hi);
      }, [=] { }, taskparts::bench_scheduler());
    } else {
      auto row_mid = (row_lo + row_hi) / 2;
      tpalrts_promote_via_nativefj([=] {
	spmv_interrupt(val, row_ptr, col_ind, x, y, row_lo, row_mid);
      }, [=] {
	spmv_interrupt(val, row_ptr, col_ind, x, y, row_mid, row_hi);
      }, [=] { }, taskparts::bench_scheduler());
    }
}

void taskparts_tpal_handler __rf_handle_col_loop(
  double* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  double* x,
  double* y,
  uint64_t row_lo,
  uint64_t row_hi,
  uint64_t col_lo,
  uint64_t col_hi,
  double t, bool& promoted) {
  
  auto nb_rows = row_hi - row_lo;
  if (nb_rows == 0) {
    promoted = false;
  } else {
    promoted = true;
    col_loop_spawn(val, row_ptr, col_ind, x, y, row_lo, row_hi, col_lo, col_hi, t, nb_rows);
  }
 exit:
  taskparts_tpal_rollbackward
}

void col_loop_col_loop_spawn(
  double* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  double* x,
  double* y,
  uint64_t col_lo,
  uint64_t col_hi,
  double t,
  double* dst) {
    auto col_mid = (col_lo + col_hi) / 2;
    std::pair<double, double> fdst;
    auto dstp = &fdst;
    tpalrts_promote_via_nativefj([&] {
      spmv_interrupt_col_loop(val, row_ptr, col_ind, x, y, col_lo, col_mid, t, &(dstp->first));
    }, [&] {
      spmv_interrupt_col_loop(val, row_ptr, col_ind, x, y, col_mid, col_hi, 0.0, &(dstp->second));
    }, [&] { 
      *dst = dstp->first + dstp->second;
    }, taskparts::bench_scheduler());
}

void taskparts_tpal_handler __rf_handle_col_loop_col_loop(
  double* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  double* x,
  double* y,
  uint64_t col_lo,
  uint64_t col_hi,
  double t,
  double* dst, bool& promoted) {
  
  if ((col_hi - col_lo) <= 1) {
    promoted = false;
  } else {
    promoted = true;
    col_loop_col_loop_spawn(val, row_ptr, col_ind, x, y, col_lo, col_hi, t, dst);
  }
  taskparts_tpal_rollbackward
}

/* Outlined-loop functions */
/* ======================= */

#define D 1024

void spmv_interrupt(
  double* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  double* x,
  double* y,
  uint64_t row_lo,
  uint64_t row_hi) {
  if (! (row_lo < row_hi)) { // row loop
    return;
  }
  for (; ; ) { 
    double r = 0.0;
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
      if (taskparts_tpal_unlikely(promoted)) {
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
    if (taskparts_tpal_unlikely(promoted)) {
      return;
    }
  }
}

void spmv_interrupt_col_loop(
  double* val,
  uint64_t* row_ptr,
  uint64_t* col_ind,
  double* x,
  double* y,
  uint64_t col_lo,
  uint64_t col_hi,
  double r,
  double* dst) {
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
    if (taskparts_tpal_unlikely(promoted)) {
      return;
    }
  }
exit:
  *dst = r;
}

char* random_bigrows_input = "random_bigrows";
char* random_bigcols_input = "random_bigcols";
char* arrowhead_input = "arrowhead";

uint64_t n_bigrows = 3000000;
//uint64_t n_bigrows = 300000;
uint64_t degree_bigrows = 100;
uint64_t n_bigcols = 23;
uint64_t n_arrowhead = 100000000;
  
uint64_t row_len = 1000;
uint64_t degree = 4;
uint64_t dim = 5;
uint64_t nb_rows;
uint64_t nb_vals;
double* val;
uint64_t* row_ptr;
uint64_t* col_ind;
double* _spmv_x;
double* _spmv_y;

uint64_t hash64(uint64_t u) {
  uint64_t v = u * 3935559000370003845ul + 2691343689449507681ul;
  v ^= v >> 21;
  v ^= v << 37;
  v ^= v >>  4;
  v *= 4768777513237032717ul;
  v ^= v << 20;
  v ^= v >> 41;
  v ^= v <<  5;
  return v;
}

auto rand_double(size_t i) -> double {
  int m = 1000000;
  double v = hash64(i) % m;
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
	j = ((h = hash64(h)) % num_rows);
      } while (j == i);
    } else {
      size_t _pow = dim+2;
      size_t h = k;
      do {
	while ((((h = hash64(h)) % 1000003) < 500001)) _pow += dim;
	j = (i + ((h = hash64(h)) % (((long) 1) << _pow))) % num_rows;
      } while (j == i);
    }
    edges.push_back(std::make_pair(i, j));
  }
  return edges;
}

  /*
static float powerlaw_random(size_t i, float dmin, float dmax, float n) {
  float r = (float)hash64(i) / RAND_MAX;
  return mypow((mypow(dmax, n) - mypow(dmin, n)) * mypow(r, 3) + mypow(dmin, n), 1.0 / n);
}
std::vector<int> siteSizes(size_t n) {
  std::vector<int> site_sizes;
  for (int i = 0; i < n;) {
    int c = powerlaw_random(i, 1,
                            std::min(50000, (int) (100000. * n / 100e6)),
                            0.001);
    c = (c==0)?1:c;
    site_sizes.push_back(c);
    i += c;
  }
  return site_sizes;
}
  */
  
auto mk_powerlaw_edgelist(size_t lg) {
  edgelist_type edges;
  size_t o = 0;
  size_t m = 1 << lg;
  size_t n = 1;
  size_t tot = m;
  for (size_t i = 0; i < lg; i++) {
    for (size_t j = 0; j < n; j++) {
      for (size_t k = 0; k < m; k++) {
	auto d = hash64(o + k) % tot;
	edges.push_back(std::make_pair(o, d));
      }
      o++;
    }
    n *= 2;
    m /= 2;
  }
  
  return edges; 
  /*
  size_t n = num_rows;
  size_t inRatio = 10;
  size_t degree = 15;
  std::vector<int> site_sizes = siteSizes(n);
  size_t sites = site_sizes.size();
  size_t *sizes = (size_t*)malloc(sizeof(size_t) * sites);
  size_t *offsets = (size_t*)malloc(sizeof(size_t) * sites);
  size_t o = 0;
  for (size_t i=0; i < sites; i++) {
    sizes[i] = site_sizes[i];
    offsets[i] = o;
    o += sizes[i];
    if (o > n) sizes[i] = std::max<size_t>(0,sizes[i] + n - o);
  }
  for (size_t i=0; i < sites; i++) {
    for (size_t j =0; j < sizes[i]; j++) {
      for (size_t k = 0; k < degree; k++) {
	auto h1 = hash64(i+j+k);
	auto h2 = hash64(h1);
	auto h3 = hash64(h2);
	size_t target_site = (h1 % inRatio != 0) ? i : (h2 % sites);
	size_t site_id = h3 % sizes[target_site];
	size_t idx = offsets[i] + j;
	edges.push_back(std::make_pair(idx, offsets[target_site] + site_id));
      }
    }
  }
  free(sizes);
  free(offsets);
  return edges; */
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
    val = (double*)malloc(sizeof(double) * nb_vals);
    for (size_t i = 0; i < nb_vals; i++) {
      val[i] = rand_double(i);
    }
  }
#ifdef MCSL_NAUTILUS
  aprintf("nb_vals %lu\n", nb_vals);
  aprintf("max_col_len %lu\n", max_col_len);
  aprintf("tot_col_len %lu\n", tot_col_len);
#else
  printf("nb_vals %lu\n", nb_vals);
  printf("max_col_len %lu\n", max_col_len);
  printf("tot_col_len %lu\n", tot_col_len);
#endif
  { /*
    for (auto& e : edges) {
      std::cout << e.first << "," << e.second << " ";
    }
    std::cout << std::endl;
    for (size_t i = 0; i < nb_rows+1; i++) {
      std::cout << "r[" << i << "]=" << row_ptr[i] << " ";
    }
    std::cout << std::endl;
    for (size_t i = 0; i < nb_vals; i++) {
      std::cout << "c[" << i << "]=" << col_ind[i] << " ";
    }
    std::cout << std::endl;  */
  } 
}

template <typename Gen_matrix>
auto bench_pre_shared(const Gen_matrix& gen_matrix) {
  gen_matrix();
  _spmv_x = (double*)malloc(sizeof(double) * nb_rows);
  _spmv_y = (double*)malloc(sizeof(double) * nb_rows);
  if ((val == nullptr) || (row_ptr == nullptr) || (col_ind == nullptr) || (_spmv_x == nullptr) || (_spmv_y == nullptr)) {
    exit(1);
  }
  // initialize x and y vectors
  {
    for (size_t i = 0; i < nb_rows; i++) {
      _spmv_x[i] = rand_double(i);
      _spmv_y[i] = 0.0;
    }
    //    tpalrts::zero_init(y, nb_rows);
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

int main() {
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    spmv_interrupt(val, row_ptr, col_ind, _spmv_x, _spmv_y, 0, nb_rows);
  }, [&] (auto sched) {
    //    spmv_manual_col_T = taskparts::cmdline::parse_or_default_int("spmv_manual_col_T", spmv_manual_col_T);
    taskparts::cmdline::dispatcher d;
    d.add("bigrows", [&] {
      n_bigrows = taskparts::cmdline::parse_or_default_long("n", n_bigrows);
      degree_bigrows = taskparts::cmdline::parse_or_default_long("degree", std::max((uint64_t)2, degree_bigrows));
      bench_pre_bigrows();
    });
    d.add("bigcols", [&] {
      n_bigcols = taskparts::cmdline::parse_or_default_long("n", n_bigcols);
      bench_pre_bigcols();
    });
    d.add("arrowhead", [&] {
      n_arrowhead = taskparts::cmdline::parse_or_default_long("n", n_arrowhead);
      bench_pre_arrowhead();
    });
    d.dispatch_or_default("inputname", "bigrows");
  }, [&] (auto sched) {
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
    free(_spmv_x);
    free(_spmv_y);
  });
  
  return 0;
}

