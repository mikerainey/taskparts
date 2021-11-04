#include "cilk.hpp"
#include "samplesort.hpp"

int main() {
  size_t repeat = taskparts::cmdline::parse_or_default_long("repeat", 0);
  if (repeat == 0) {
    taskparts::benchmark_cilk([&] {
      benchmark_no_swaps();
    }, [&] {
      gen_input();
    });
  } else {
    taskparts::benchmark_cilk([&] {
      benchmark_with_swaps(repeat);
    }, [&] {
      gen_input();
    });
  }
  return 0;
}
