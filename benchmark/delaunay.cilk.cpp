#include "cilk.hpp"
#include "delaunay.hpp"

int main() {
  taskparts::benchmark_cilk([&] { // benchmark
    benchmark();
  }, [&] {
    gen_input();
  }, [&] { // teardown
    // later: write results to outfile
  }, [&] { // reset
    reset();    
  });
  return 0;
}
