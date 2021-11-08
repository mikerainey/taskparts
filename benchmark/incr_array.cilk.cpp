#include "cilk.hpp"

void incr_array_cilk(int32_t* a, uint64_t lo, uint64_t hi) {
  cilk_for (uint64_t i = 0; i != hi; i++) {
    a[i]++;
  }
}

int main() {
  size_t nb_items = taskparts::cmdline::parse_or_default_long("n", 100 * 1000 * 1000);
  int32_t* a = nullptr;
  taskparts::benchmark_cilk([&] {
    incr_array_cilk(a, 0, nb_items);
  }, [&] {
    a = new int32_t[nb_items];
    for (size_t i = 0; i < nb_items; i++) {
      a[i] = 1.0;
    }
  }, [&] {
    printf("result=%d\n",a[0]);
    delete [] a;
  });
  return 0;
}
