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
  std::array<Fiber*, max_sz> backup_deq;
  
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

  auto relocate() -> age_t {
    auto freeze = [&] () -> age_t {
      auto orig = age.load();
      auto next = orig;
      while (true) {
	next.top = 0;
	next.bot = 0;
	next.tag = orig.tag + 1;
	if (age.compare_exchange_strong(orig, next)) {
	  break;
	}
      }
      return orig;
    };
    auto backup = [&] (age_t orig) -> age_t {
      qidx j = 0;
      for (qidx i = orig.top; i < orig.bot; i++) {
	backup_deq[j++] = deq[i].f.load(std::memory_order_relaxed);
      }
      assert(j == size(orig));
      return orig;
    };
    auto restore = [&] (age_t orig) -> age_t {
      auto n = size(orig);
      for (qidx i = 0; i < n; i++) {
	deq[i].f.store(backup_deq[i], std::memory_order_relaxed);
      }
      orig.top = 0;
      orig.bot = n;
      orig.tag++;
      return orig;
    };
    return restore(backup(freeze()));
  }

  auto reset_on_sz_zero(age_t orig) -> age_t {
    if (size(orig) > 0) {
      return orig;
    }
    auto next = orig;
    next.top = 0;
    next.bot = 0;
    next.tag = orig.tag + 1;
    if (! age.compare_exchange_strong(orig, next)) {
      taskparts_die("bogus");
    }
    return next;
  }
  
  auto push(Fiber* f) -> deque_surplus_result_type {
    auto orig = reset_on_sz_zero(age.load());
    auto next = orig;
    next.bot++;
    if (next.bot == max_sz) {
      if (size(orig) == max_sz) {
	taskparts_die("internal error: scheduler queue overflow\n");
      }
      orig = relocate();
    }
    next.tag = orig.tag + 1;
    deq[orig.bot].f.store(f, std::memory_order_relaxed);
    while (true) {
      if (age.compare_exchange_strong(orig, next)) {
        break;
      }
      next.top = orig.top;
      next.tag = orig.tag + 1;
    }
    assert(size(orig) + 1 == size(next));
    return (size(orig) == 0) ? deque_surplus_up : deque_surplus_stable;
  }
  
  auto pop() -> std::pair<Fiber*, deque_surplus_result_type> {
    auto orig = reset_on_sz_zero(age.load());
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
