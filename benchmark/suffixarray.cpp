#include <taskparts/benchmark.hpp>
#include <parlay/delayed_sequence.h>
#include <parlay/monoid.h>
#include <parlay/primitives.h>
#include <parlay/parallel.h>
#include <common/IO.h>
#include <common/sequenceIO.h>
#include <suffixArray/bench/SA.h>
#include <suffixArray/parallelKS/SA.C>
using uchar = unsigned char;

int main() {
  std::string fname = taskparts::cmdline::parse_or_default_string("infile", "chr22.dna");

  parlay::sequence<uchar> ss;
  parlay::sequence<indexT> R;
  parlay::benchmark_taskparts([&] (auto sched) {
    R = suffixArray(ss);
  }, [&] (auto sched) {
    parlay::sequence<char> s = benchIO::readStringFromFile(fname.c_str());
    ss = parlay::tabulate(s.size(), [&] (size_t i) -> uchar {return (uchar) s[i];});
  });
  return 0;
}
