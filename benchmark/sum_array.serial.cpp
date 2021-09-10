#include "benchmark.hpp"

#include "sum_array_rollforward_decls.hpp"

double sum_array_serial(double* a, uint64_t lo, uint64_t hi);
void sum_array_heartbeat(double* a, uint64_t lo, uint64_t hi, double r, double* dst);

int sum_array_heartbeat_handler(double* a, uint64_t lo, uint64_t hi, double r, double* dst) { return 0; }

int main() {
  uint64_t nb_items = 100 * 1000 * 1000;
  double result = 0.0;
  double* a = new double[nb_items];
  for (size_t i = 0; i < nb_items; i++) {
    a[i] = 1.0;
  }

  taskparts::run_benchmark([&] {
    result = sum_array_serial(a, 0, nb_items);
  });
  
  printf("result=%f\n",result);
  delete [] a;
  return 0;
}
