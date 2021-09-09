#include "benchmark.hpp"

double sum_array_serial(double* a, uint64_t lo, uint64_t hi);

int main() {
  uint64_t nb_items = 100 * 1000 * 1000;
  double result = 0.0;
  double* a = new double[nb_items];
  for (size_t i = 0; i < nb_items; i++) {
    a[i] = 1.0;
  }

  taskparts::run_benchmark([&] {
    sum_array_serial(a, 0, nb_items, 0.0, &result);
  });
  
  printf("result=%f\n",result);
  delete [] a;
  return 0;
}
