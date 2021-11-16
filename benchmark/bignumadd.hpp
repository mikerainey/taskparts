#pragma once

#include "common.hpp"
#include "common/get_time.h"

using byte = unsigned char;

// bignum represented as sequence of bytes, each with values in the range [0..128)
// i.e. think of as radix 128 integer representation
using bignum = parlay::sequence<byte>;

auto big_add_delayed(bignum const& A, bignum const &B) {
  timer t("add");
  size_t n = A.size();
  auto sums = parlay::delayed_seq<byte>(n, [&] (size_t i) -> byte {
      return A[i] + B[i];});
  auto f = [] (byte a, byte b) ->byte { // carry propagate
    return (b == 127) ? a : b;};
  auto [carries, total] = parlay::block_delayed::scan(sums, parlay::make_monoid(f,(byte)0));
  t.next("scan");
  auto z = parlay::block_delayed::zip(carries, sums);
  auto mr = parlay::block_delayed::map(z, [] (auto x) {return ((x.first >> 7) + x.second) & 127;});
  auto r = parlay::block_delayed::force(mr);
  t.next("force");
  return std::pair(std::move(r), (total >> 7));
}

int result1, result2;
parlay::sequence<byte> a;
parlay::sequence<byte> b;

auto benchmark() {
  auto [sums, carry] = big_add_delayed(a,b);
  result1 = sums[0];
  result2 = carry;
}

auto gen_input() {
  size_t n = std::max((size_t)1, (size_t)taskparts::cmdline::parse_or_default_long("n", 100000000));
  a = parlay::tabulate(n, [] (size_t i) -> byte {return 64;});
  b = parlay::tabulate(n, [] (size_t i) -> byte {return 64;});
}
