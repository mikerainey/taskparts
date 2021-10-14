#ifndef TASKPARTS_TPALRTS
#error "need to compile with tpal flags, e.g., TASKPARTS_TPALRTS"
#endif
#include <taskparts/benchmark.hpp>

#include "srad_rollforward_decls.hpp"
#include "srad.hpp"

extern
void srad_tpal(int rows, int cols, int size_I, int size_R, float* __restrict__ I, float* __restrict__ J, float q0sqr, float * __restrict__ dN, float * __restrict__ dS, float * __restrict__ dW, float * __restrict__ dE, float* __restrict__ c, int* __restrict__ iN, int* __restrict__ iS, int* __restrict__ jE, int* __restrict__ jW, float lambda);

extern
void srad_tpal_1(int rows, int cols, int rows_lo, int rows_hi, int size_I, int size_R, float* __restrict__ I, float* __restrict__ J, float q0sqr, float * __restrict__ dN, float * __restrict__ dS, float * __restrict__ dW, float * __restrict__ dE, float* __restrict__ c, int* __restrict__ iN, int* __restrict__ iS, int* __restrict__ jE, int* __restrict__ jW, float lambda);

extern
void srad_tpal_inner_1(int rows, int cols, int rows_lo, int rows_hi, int cols_lo, int cols_hi, int size_I, int size_R, float* __restrict__ I, float* __restrict__ J, float q0sqr, float * __restrict__ dN, float * __restrict__ dS, float * __restrict__ dW, float * __restrict__ dE, float* __restrict__ c, int* __restrict__ iN, int* __restrict__ iS, int* __restrict__ jE, int* __restrict__ jW, float lambda);

extern
void srad_tpal_2(int rows, int cols, int rows_lo, int rows_hi, int cols_lo, int cols_hi, int size_I, int size_R, float* __restrict__ I, float* __restrict__ J, float q0sqr, float * __restrict__ dN, float * __restrict__ dS, float * __restrict__ dW, float * __restrict__ dE, float* __restrict__ c, int* __restrict__ iN, int* __restrict__ iS, int* __restrict__ jE, int* __restrict__ jW, float lambda);

extern
void srad_tpal_inner_2(int rows, int cols, int rows_lo, int rows_hi, int cols_lo, int cols_hi, int size_I, int size_R, float* __restrict__ I, float* __restrict__ J, float q0sqr, float * __restrict__ dN, float * __restrict__ dS, float * __restrict__ dW, float * __restrict__ dE, float* __restrict__ c, int* __restrict__ iN, int* __restrict__ iS, int* __restrict__ jE, int* __restrict__ jW, float lambda);



int srad_handler(int rows, int cols, int rows_lo, int rows_hi, int cols_lo, int cols_hi, int size_I, int size_R, float* __restrict__ I, float* __restrict__ J, float q0sqr, float * __restrict__ dN, float * __restrict__ dS, float * __restrict__ dW, float * __restrict__ dE, float* __restrict__ c, int* __restrict__ iN, int* __restrict__ iS, int* __restrict__ jE, int* __restrict__ jW, float lambda) {
  auto nb_rows = rows_hi - rows_lo;
  if (nb_rows <= 1) {
    return 0;
  }
  auto rf = [=] {
    srad_tpal_inner_1(rows, cols, rows_lo, rows_hi, cols_lo, cols_hi, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
  };
  rows_lo++;
  if (nb_rows == 2) {
    taskparts::tpalrts_promote_via_nativefj([=] {
      rf();
      srad_tpal_1(rows, cols, rows_lo, rows_hi, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
    }, [=] {
      srad_tpal_2(rows, cols, rows_lo, rows_hi, 0, 0, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
    }, [=] { }, taskparts::bench_scheduler());
  } else {
    auto rows_mid = (rows_lo + rows_hi) / 2;
    taskparts::tpalrts_promote_via_nativefj([=] {
      rf();
      srad_tpal_1(rows, cols, rows_lo, rows_mid, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
    }, [=] {
      srad_tpal_1(rows, cols, rows_mid, rows_hi, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
      srad_tpal_2(rows, cols, 0, rows, 0, 0, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
    }, [=] { }, taskparts::bench_scheduler());    
  }
  return 1;
}

int srad_handler_1(int rows, int cols, int rows_lo, int rows_hi, int cols_lo, int cols_hi, int size_I, int size_R, float* __restrict__ I, float* __restrict__ J, float q0sqr, float *__restrict__ dN, float *__restrict__ dS, float *__restrict__ dW, float *__restrict__ dE, float* __restrict__ c, int* __restrict__ iN, int* __restrict__ iS, int* __restrict__ jE, int* __restrict__ jW, float lambda) {
  auto nb_rows = rows_hi - rows_lo;
  if (nb_rows <= 1) {
    return 0;
  }
  auto rf = [=] {
    srad_tpal_inner_1(rows, cols, rows_lo, rows_hi, cols_lo, cols_hi, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
  };
  rows_lo++;
  if (nb_rows == 2) {
    taskparts::tpalrts_promote_via_nativefj(rf, [=] {
      srad_tpal_1(rows, cols, rows_lo, rows_hi, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
    }, [=] { }, taskparts::bench_scheduler());
  } else {
    auto rows_mid = (rows_lo + rows_hi) / 2;
    taskparts::tpalrts_promote_via_nativefj([=] {
      rf();
      srad_tpal_1(rows, cols, rows_lo, rows_mid, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
    }, [=] {
      srad_tpal_1(rows, cols, rows_mid, rows_hi, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
    }, [=] { }, taskparts::bench_scheduler());    
  }
  return 1;
}

int srad_handler_inner_1(int rows, int cols, int rows_lo, int rows_hi, int cols_lo, int cols_hi, int size_I, int size_R, float* __restrict__ I, float* __restrict__ J, float q0sqr, float *__restrict__ dN, float *__restrict__ dS, float *__restrict__ dW, float *__restrict__ dE, float* __restrict__ c, int* __restrict__ iN, int* __restrict__ iS, int* __restrict__ jE, int* __restrict__ jW, float lambda) {
  if ((cols_hi - cols_lo) <= 1) {
    return 0;
  }
  auto cols_mid = (cols_lo + cols_hi) / 2;
  taskparts::tpalrts_promote_via_nativefj([=] {
    srad_tpal_inner_1(rows, cols, rows_lo, rows_hi, cols_lo, cols_mid, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
  }, [=] {
    srad_tpal_inner_1(rows, cols, rows_lo, rows_hi, cols_mid, cols_hi, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
  }, [=] { }, taskparts::bench_scheduler());
  return 1;
}

int srad_handler_2(int rows, int cols, int rows_lo, int rows_hi, int cols_lo, int cols_hi, int size_I, int size_R, float* __restrict__ I, float* __restrict__ J, float q0sqr, float *__restrict__ dN, float *__restrict__ dS, float *__restrict__ dW, float *__restrict__ dE, float* __restrict__ c, int* __restrict__ iN, int* __restrict__ iS, int* __restrict__ jE, int* __restrict__ jW, float lambda) {
  auto nb_rows = rows_hi - rows_lo;
  if (nb_rows <= 1) {
    return 0;
  }
  auto rf = [=] {
    srad_tpal_inner_2(rows, cols, rows_lo, rows_hi, cols_lo, cols_hi, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
  };
  rows_lo++;
  if (nb_rows == 2) {
    taskparts::tpalrts_promote_via_nativefj([=] {
      rf();
    }, [=] {
      srad_tpal_2(rows, cols, rows_lo, rows_hi, 0, 0, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
    }, [=] { }, taskparts::bench_scheduler());
  } else {
    auto rows_mid = (rows_lo + rows_hi) / 2;
    taskparts::tpalrts_promote_via_nativefj([=] {
      rf();
      srad_tpal_2(rows, cols, rows_lo, rows_mid, 0, 0, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
    }, [=] {
      srad_tpal_2(rows, cols, rows_mid, rows_hi, 0, 0, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
    }, [=] { }, taskparts::bench_scheduler());
  }
  return 1;
}

int srad_handler_inner_2(int rows, int cols, int rows_lo, int rows_hi, int cols_lo, int cols_hi, int size_I, int size_R, float* __restrict__ I, float* __restrict__ J, float q0sqr, float *__restrict__ dN, float *__restrict__ dS, float *__restrict__ dW, float *__restrict__ dE, float* __restrict__ c, int* __restrict__ iN, int* __restrict__ iS, int* __restrict__ jE, int* __restrict__ jW, float lambda) {
  if ((cols_hi - cols_lo) <= 1) {
    return 0;
  }
  auto cols_mid = (cols_lo + cols_hi) / 2;
  taskparts::tpalrts_promote_via_nativefj([=] {
    srad_tpal_inner_2(rows, cols, rows_lo, rows_hi, cols_lo, cols_mid, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
  }, [=] {
    srad_tpal_inner_2(rows, cols, rows_lo, rows_hi, cols_mid, cols_hi, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
  }, [=] {
    // nothing left to do
  }, taskparts::bench_scheduler());
  return 1;
}

namespace taskparts {
auto initialize_rollforward() {
  rollforward_table = {
    #include "srad_rollforward_map.hpp"
  };
  initialize_rollfoward_table();
}
} // end namespace

int main() {
  taskparts::initialize_rollforward();
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    srad_tpal(rows, cols, size_I, size_R, I, J, q0sqr, dN, dS, dW, dE, c, iN, iS, jE, jW, lambda);
  }, [&] (auto sched) {
    bench_pre();    
  }, [&] (auto sched) {
    print_summary();
    bench_post();
  }, [&] (auto sched) {
    init_J();
  });
  return 0;
}
