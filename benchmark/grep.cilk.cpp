#include "cilk.hpp"
#include "grep.hpp"

int main() {
  taskparts::benchmark_cilk([&] { // benchmark
    benchmark();
  }, [&] { // setup
    gen_input();
  }, [&] { // teardown
    std::cout << "result " << result << std::endl;
  }, [&] { // reset

  });
  return 0;
}

