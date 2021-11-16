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

auto big_add_rad(bignum const& A, bignum const &B) {
  timer t("add");
  size_t n = A.size();
  auto sums = parlay::delayed_tabulate(n, [&] (size_t i) {
      return A[i] + B[i];});
  t.next("sums");
  auto f = [] (byte a, byte b) { // carry propagate
    return (b == 127) ? a : b;};
  auto [carries, total] = parlay::scan(sums, parlay::make_monoid(f,0));
  t.next("scan");
  auto r = parlay::tabulate(n, [&] (size_t i) {
      return ((carries[i] >> 7) + sums[i]) & 127;});
  t.next("tabulate");
  return std::pair(std::move(r), (total >> 7));
}

auto big_add_strict(bignum const& A, bignum const &B) {
  timer t("add");
  size_t n = A.size();
  auto sums = parlay::tabulate(n, [&] (size_t i) {
      return A[i] + B[i];});
  t.next("sums");
  auto f = [] (byte a, byte b) { // carry propagate
    return (b == 127) ? a : b;};
  auto [carries, total] = parlay::scan(sums, parlay::make_monoid(f,0));
  t.next("scan");
  auto r = parlay::tabulate(n, [&] (size_t i) {
      return ((carries[i] >> 7) + sums[i]) & 127;});
  t.next("tabulate");
  return std::pair(std::move(r), (total >> 7));
}
