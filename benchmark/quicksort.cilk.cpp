#include "cilk.hpp"
#include "quicksort.hpp"

int main() {
  taskparts::benchmark_cilk([&] {
    benchmark();
  }, [&] {
    gen_input();
  });
  return 0;
}
