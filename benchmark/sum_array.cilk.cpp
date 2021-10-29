#include "cilk.hpp"

double sum_array_cilk(double* __restrict__ a, uint64_t lo, uint64_t hi) {
  cilk::reducer_opadd<double> sum(0);
  cilk_for (uint64_t i = 0; i != hi; i++) {
    *sum += a[i];
  }
  return sum.get_value();
}

int main() {
  size_t nb_items = taskparts::cmdline::parse_or_default_long("n", 100 * 1000 * 1000);
  double result = 0.0;
  double* a = nullptr;
  taskparts::benchmark_cilk([&] {
    result = sum_array_cilk(a, 0, nb_items);
  }, [&] {
    a = new double[nb_items];
    for (size_t i = 0; i < nb_items; i++) {
      a[i] = 1.0;
    }
  }, [&] {
    printf("result=%f\n",result);
    delete [] a;
  });
  return 0;
}
