#include "taskparts/chaselev.hpp"
#include "taskparts/tpalrts.hpp"

#include "sum_array_rollforward_decls.hpp"

double sum_array_serial(double* a, uint64_t lo, uint64_t hi);

void sum_array_heartbeat(double* a, uint64_t lo, uint64_t hi, double r, double* dst);

int sum_array_heartbeat_handler(double* a, uint64_t lo, uint64_t hi, double r, double* dst) {
  return 0;
}

uint64_t nb_items = 100 * 1000 * 1000;
double* a;
double result = 0.0;

namespace taskparts {
void init() {
  taskparts::rollforward_table = {
    #include "sum_array_rollforward_map.hpp"
  };
  uint64_t my_cpu_freq_khz = 3500000;
  assign_kappa(my_cpu_freq_khz);
}
}

int main() {
  taskparts::init();
  double* a = new double[nb_items];
  for (size_t i = 0; i < nb_items; i++) {
    a[i] = 1.0;
  }
  sum_array_heartbeat(a, 0, nb_items, 0.0, &result);
  printf("result=%f\n",result);
  return 0;
}
