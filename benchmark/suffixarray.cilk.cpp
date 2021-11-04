#include "cilk.hpp"
#include "suffixarray.hpp"

int main() {
  include_input_load = taskparts::cmdline::parse_or_default_bool("include_input_load", true);
  taskparts::benchmark_cilk([&] {
    benchmark();
  }, [&] {
    if (! include_input_load) {
      load_input();
    }
  });
  return 0;
}
