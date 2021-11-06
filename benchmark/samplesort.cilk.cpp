#include "cilk.hpp"
#include "samplesort.hpp"

int main() {
  taskparts::benchmark_cilk([&] {
    benchmark();
  }, [&] {
    gen_input();
  });
  return 0;
}
