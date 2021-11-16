#include "cilk.hpp"
#include "bignumadd.hpp"

int main() {
  taskparts::benchmark_cilk([&] { // benchmark
    benchmark();
  }, [&] { // setup
    gen_input();
  }, [&] { // teardown
    std::cout << "result1 " << result1 << std::endl;
    std::cout << "result2 " << result2 << std::endl;
  }, [&] { // reset

  });
  return 0;
}
