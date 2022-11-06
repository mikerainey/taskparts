#include "cilk.hpp"
#include "comparisonsort.hpp"

int main() {
  taskparts::benchmark_cilk([&] {
    benchmark();
  }, [&] {
    gen_input();
  });
  return 0;
}
