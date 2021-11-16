#include "cilk.hpp"
#include "mcss.hpp"

int main() {
  taskparts::benchmark_cilk([&] { // benchmark
    benchmark();
  }, [&] { // setup
    gen_input();
  });
  return 0;
}
