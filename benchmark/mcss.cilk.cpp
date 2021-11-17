#include "cilk.hpp"
#include "mcss.hpp"

int main() {
  taskparts::benchmark_cilk([&] { // benchmark
    benchmark();
  }, [&] { // setup
    gen_input();
  }, [&] { // teardown
    printf("result %f\n", result);
  });
  return 0;
}
