#include "common.hpp"
#include <parlay/io.h>
#include <parlay/primitives.h>
#include <parlay/sequence.h>
#include <parlay/internal/get_time.h>
#include "longest_repeated_substring.h"

using uchar = unsigned char;
using charseq = parlay::sequence<char>;
using uint = unsigned int;

charseq str;
std::tuple<uint,uint,uint> result;
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
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  std::string fname = taskparts::cmdline::parse_or_default_string("input", "chr22.dna");
  str = parlay::chars_from_file(fname.c_str());
  SA = suffix_array(str);
}

auto benchmark_dflt2() {
  if (include_infile_load) {
    gen_input2();
  }
  result = longest_repeated_substring(str);
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
    auto [len, l1, l2] = result;
    std::cout << "longest match has length = " << len
              << " at positions " << l1 << " and " << l2 << std::endl;
    std::cout << to_chars(to_sequence(str.cut(l1, l1 + std::min(2000u,len)))) << std::endl;
    if (len > 2000) std::cout << "...." << std::endl; 
#endif
  }, [&] (auto sched) { // reset
    SA = suffix_array(str);
  });
  return 0;
}
