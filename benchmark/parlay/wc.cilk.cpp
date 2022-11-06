#include "cilk.hpp"
#include <taskparts/benchmark.hpp>
#include "wc.hpp"

int main() {
  taskparts::benchmark_cilk([&] {
    benchmark();
  }, [&] { // setup
    gen_input();
  });
  return 0;
}
