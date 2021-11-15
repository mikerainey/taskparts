#pragma once

#include "common.hpp"

std::vector<long long> a;

// ------------- Maximum Contiguous Subsequence Sum ------------------

// 10x improvement by using delayed sequence
template <class Seq>
typename Seq::value_type mcss(Seq const &A) {
  using T = typename Seq::value_type;
  auto f = [&] (auto a, auto b) {
    auto [aa, pa, sa, ta] = a;
    auto [ab, pb, sb, tb] = b;
    return std::make_tuple(
      std::max(aa,std::max(ab,sa+pb)),
      std::max(pa, ta+pb),
      std::max(sa + ab, sb),
      ta + tb
    );
  };
  auto S = parlay::delayed_seq<std::tuple<T,T,T,T>>(A.size(), [&] (size_t i) {
    return std::make_tuple(A[i],A[i],A[i],A[i]);
  });
  auto m = parlay::make_monoid(f, std::tuple<T,T,T,T>(0,0,0,0));
  auto r = parlay::reduce(S, m);
  return std::get<0>(r);
}

auto gen_input() {
  size_t n = taskparts::cmdline::parse_or_default_long("n", 500 * 1000 * 1000);
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  a.resize(n);
  parlay::parallel_for(0, n, [&] (auto i) {
    a[i] = (i % 2 == 0 ? -1 : 1) * i;
  });
}


auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  mcss(a);
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}
