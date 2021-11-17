#pragma once

#include "common.hpp"

parlay::sequence<double> a;
double result;

// ------------- Maximum Contiguous Subsequence Sum ------------------

auto mcss(parlay::sequence<double> const& A) {
  using tu = std::array<double,4>;
  auto f = [] (tu a, tu b) {
    tu r = {std::max(std::max(a[0],b[0]),a[2]+b[1]),
	    std::max(a[1],a[3]+b[1]),
	    std::max(a[2]+b[3],b[2]),
	    a[3]+b[3]};
    return r;};
  double neginf = std::numeric_limits<double>::lowest();
  tu identity = {neginf, neginf, neginf, (double) 0.0};
  auto pre = parlay::delayed_seq<tu>(A.size(), [&] (size_t i) -> tu {
      tu x = {A[i],A[i],A[i],A[i]};
      return x;
    });
  auto r = parlay::reduce(pre, parlay::make_monoid(f, identity));
  return r[0];
}

auto gen_input() {
  size_t n = taskparts::cmdline::parse_or_default_long("n", 500 * 1000 * 1000);
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  a = parlay::tabulate(n, [] (size_t i) -> double {return 1.0;});
}


auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  result = mcss(a);
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}
