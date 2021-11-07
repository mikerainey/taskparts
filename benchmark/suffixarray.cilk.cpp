#include "cilk.hpp"
#include "suffixarray.hpp"

int main() {
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  taskparts::benchmark_cilk([&] {
    benchmark();
  }, [&] {
    if (! include_infile_load) {
      load_input();
    }
  });
  return 0;
}
