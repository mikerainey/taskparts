#include <taskparts/benchmark.hpp>

double sum_array_serial(double* __restrict__ a, uint64_t lo, uint64_t hi) {
  double r = 0.0;
  for (uint64_t i = lo; i != hi; i++) {
    r += a[i];
  }
  return r;
}

int main() {
  size_t nb_items = taskparts::cmdline::parse_or_default_long("n", 100 * 1000 * 1000);
  double result = 0.0;
  double* a = nullptr;
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    result = sum_array_serial(a, 0, nb_items);
  }, [&] (auto sched) {
    a = new double[nb_items];
    for (size_t i = 0; i < nb_items; i++) {
      a[i] = 1.0;
    }
  }, [&] (auto sched) {
    printf("result=%f\n",result);
    delete [] a;
  });
  return 0;
}
