#include <taskparts/benchmark.hpp>
#include "suffixarray.hpp"
#include "longest_common_prefix.h"

using charseq = parlay::sequence<char>;
using uint = unsigned int;

long n;
parlay::sequence<uint> SA;

template <class Seq1, class Seq2, class Seq3>
auto check(const Seq1 &s, const Seq2 &SA, const Seq3& lcp) {
  long n = s.size();
  long l = parlay::count(parlay::tabulate(lcp.size(), [&] (long i) {
    long j = 0;
    while (j < n - SA[i] && (s[SA[i]+j] == s[SA[i+1]+j])) j++;
    return j == lcp[i];}), true);
  return l == lcp.size();
}

auto gen_input2() {
  gen_input();
  SA = suffix_array(str);
}

auto benchmark_dflt2() {
  if (include_infile_load) {
    gen_input2();
  }
  result = lcp(str,SA);
}

auto benchmark2() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt2(); });
  } else {
    benchmark_dflt2();
  }
}

int main() {
  parlay::benchmark_taskparts([&] (auto sched) { // benchmark
    benchmark2();
  }, [&] (auto sched) { // setup
    gen_input2();
  }, [&] (auto sched) { // teardown
#ifndef NDEBUG
    // check correctness
    if (!check(str, SA, result))
      std::cout << "check failed" << std::endl;
#endif
  }, [&] (auto sched) { // reset
    result.clear();
  });
  return 0;
}
