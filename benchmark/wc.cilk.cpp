#include "cilk.hpp"
#include "wc.hpp"

int main() {
  taskparts::benchmark_cilk([&] {
    benchmark();
  }, [&] {
    gen_input();
  });
  return 0;
}
