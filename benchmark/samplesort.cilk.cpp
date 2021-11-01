#include "cilk.hpp"
#include "samplesort.hpp"

int main() {
  taskparts::benchmark_cilk([&] {
    compSort(a, [] (item_type x, item_type y) { return x < y; });
  }, [&] {
    gen_input();
  });
  return 0;
}
