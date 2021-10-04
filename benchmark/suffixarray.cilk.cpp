#include "cilk.hpp"
#include "suffixarray.hpp"

int main() {
  std::string fname = taskparts::cmdline::parse_or_default_string("infile", "chr22.dna");

  parlay::sequence<uchar> ss;
  parlay::sequence<indexT> R;
  taskparts::benchmark_cilk([&] {
    R = suffixArray(ss);
  }, [&] {
    parlay::sequence<char> s = benchIO::readStringFromFile(fname.c_str());
    ss = parlay::tabulate(s.size(), [&] (size_t i) -> uchar {return (uchar) s[i];});
  });
  return 0;
}
