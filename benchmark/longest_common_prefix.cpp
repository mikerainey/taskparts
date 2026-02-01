#include <algorithm>
#include <ctype.h>
#include <iostream>
#include <string>

#include <parlay/internal/get_time.h>
#include <parlay/io.h>
#include <parlay/primitives.h>
#include <parlay/sequence.h>

// need suffix array to generate sample
#include "benchmark.hpp"
#include "longest_common_prefix.h"
#include "suffix_array.h"

// **************************************************************
// Driver code
// **************************************************************
using charseq = parlay::sequence<char>;
using uint = unsigned int;

template <class Seq1, class Seq2, class Seq3>
auto check(const Seq1 &s, const Seq2 &SA, const Seq3 &lcp) {
  long n = s.size();
  long l = parlay::count(
      parlay::tabulate(lcp.size(),
                       [&](long i) {
                         long j = 0;
                         while (j < n - SA[i] &&
                                (s[SA[i] + j] == s[SA[i + 1] + j]))
                           j++;
                         return j == lcp[i];
                       }),
      true);
  return l == lcp.size();
}

int main(int argc, char *argv[]) {
  auto usage = "Usage: longest_common_prefix <filename>";
  if (argc != 2)
    std::cout << usage << std::endl;
  else {
    charseq str = parlay::chars_from_file(argv[1]);
    using index = unsigned int;
    long n = str.size();
    auto SA = suffix_array(str);
    parlay::sequence<index> result;

    taskparts::benchmark([&] { result = lcp(str, SA); });

    // check correctness
    if (!check(str, SA, result))
      std::cout << "check failed" << std::endl;
  }
}
