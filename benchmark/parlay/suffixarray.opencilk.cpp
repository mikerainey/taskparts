#include "opencilk.hpp"
#include <taskparts/benchmark.hpp>
#include "suffixarray.hpp"

int main() {
  taskparts::benchmark_cilk([&] {
    benchmark();
  }, [&] { // setup
    gen_input();
  });
  return 0;
}
