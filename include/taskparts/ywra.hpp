#pragma once

#include <atomic>
#include <memory>
#include <assert.h>
#include <array>

#include "perworker.hpp"
#include "fixedcapacity.hpp"
#include "scheduler.hpp"
#include "hash.hpp"

namespace taskparts {

// Deque from Yue Yao, Sam Westrick, Mike Rainey, and Umut Acar (2022)
template <typename Fiber>
struct ywra {
  
  using qidx = uint16_t;
  
  // use std::atomic<age_t> for atomic access.
  // Note: Explicit alignment specifier required
  // to ensure that Clang inlines atomic loads.
  struct alignas(int64_t) age_t {
    uint32_t tag;
    qidx bot;
    qidx top;
  };
  
  // align to avoid false sharing
  struct alignas(64) padded_fiber {
    std::atomic<Fiber*> f;
  };

  static constexpr
  int max_sz = (1 << 14); // can in principle be up to 2^16
  
  std::atomic<age_t> age;
  
  std::array<padded_fiber, max_sz> deq;
  
  ywra() : age(age_t{0, 0, 0}) { }
  
  auto size(age_t a) -> unsigned int {
    assert(a.bot >= a.top);
    return a.bot - a.top;
  }

  auto size() -> unsigned int {
    return size(age.load());
  }
  
  auto empty() -> bool {
    return size() == 0;
  }
  
  auto push(Fiber* f) -> deque_surplus_result_type {
    auto orig = age.load();
    auto next = orig;
    next.bot++;
    if (next.bot == max_sz) {
      throw std::runtime_error("internal error: scheduler queue overflow");
    }
    next.tag = orig.tag + 1;
    deq[orig.bot].f.store(f, std::memory_order_relaxed);
    while (true) {
      if (age.compare_exchange_strong(orig, next)) {
        break;
      }
      next.top = orig.top;
      next.tag = orig.tag + 1;
      if (size(next) == 0) {
	deq[0].f.store(f, std::memory_order_relaxed);
        next.top = 0;
        next.bot = 1;
      }
    }
    assert(size(orig) + 1 == size(next));
    return (size(orig) == 0) ? deque_surplus_up : deque_surplus_stable;
  }
  
  auto pop() -> std::pair<Fiber*, deque_surplus_result_type> {
    auto orig = age.load();
    if (size(orig) == 0) {
      return std::make_pair(nullptr, deque_surplus_stable);
    }
    auto next = orig;
    next.bot--;
    next.tag = orig.tag + 1;
    auto f = deq[next.bot].f.load(std::memory_order_relaxed);
    assert(f != nullptr);
    while (true) {
      if (age.compare_exchange_strong(orig, next)) {
        break;
      }
      next.top = orig.top;
      next.tag = orig.tag + 1;
      assert((orig.bot - 1) == next.bot);
      if (size(orig) == 0) {
        return std::make_pair(nullptr, deque_surplus_stable);
      }
    }
    auto r = (size(orig) == 1) ? deque_surplus_down : deque_surplus_stable;
    return std::make_pair(f, r);
  }
  
  auto steal() -> std::pair<Fiber*, deque_surplus_result_type> {
    auto orig = age.load();
    if (size(orig) == 0) {
      return std::make_pair(nullptr, deque_surplus_stable);
    }
    auto next = orig;
    auto f = deq[next.top].f.load(std::memory_order_relaxed);
    next.top++;
    next.tag = orig.tag + 1;
    if (! age.compare_exchange_strong(orig, next)) {
      return std::make_pair(nullptr, deque_surplus_stable);
    }
    auto r = (size(orig) == 1) ? deque_surplus_down : deque_surplus_stable;
    return std::make_pair(f, r);
  }
  
};

} // end namespace
