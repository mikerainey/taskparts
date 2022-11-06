#include "cilk.hpp"
#include "removeduplicates.hpp"

int main() {
  taskparts::benchmark_cilk([&] { // benchmark
    benchmark();
  }, [&] { // setup
    gen_input();
  }, [&] { // teardown
    // later: write results to outfile
  }, [&] { // reset
    reset();    
  });
  return 0;
}
