#include "cilk.hpp"
#include "matching.hpp"

int main() {
  taskparts::benchmark_cilk([&] {
    benchmark();
  }, [&] {
    gen_input();
  });
  return 0;
}
