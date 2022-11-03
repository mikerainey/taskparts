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

using deque_surplus_result_type = enum deque_surplus_result_enum {
  deque_surplus_stable, deque_surplus_up, deque_surplus_down,
  deque_surplus_unknown
};

#ifndef TASKPARTS_ELASTIC_SURPLUS

// Deque from Arora, Blumofe, and Plaxton (SPAA, 1998).
template <typename Fiber>
struct abp_deque {
  using qidx = unsigned int;
  using tag_t = unsigned int;
  
  // use std::atomic<age_t> for atomic access.
  // Note: Explicit alignment specifier required
  // to ensure that Clang inlines atomic loads.
  struct alignas(int64_t) age_t {
    // cppcheck bug prevents it from seeing usage with braced initializer
    tag_t tag;                // cppcheck-suppress unusedStructMember
    qidx top;                 // cppcheck-suppress unusedStructMember
  };
  
  // align to avoid false sharing
  struct alignas(64) padded_fiber {
    std::atomic<Fiber*> f;
  };
  
  static constexpr int q_size = 1000;
  std::atomic<qidx> bot;
  std::atomic<age_t> age;
  std::array<padded_fiber, q_size> deq;
  
  abp_deque() : bot(0), age(age_t{0, 0}) {}
  
  auto size() -> unsigned int {
    return bot.load() - age.load().top;
  }
  
  auto empty() -> bool {
    return size() == 0;
  }
  
  auto push(Fiber* f) -> deque_surplus_result_type {
    auto local_bot = bot.load(std::memory_order_relaxed);      // atomic load
    deq[local_bot].f.store(f, std::memory_order_relaxed);  // shared store
    local_bot += 1;
    if (local_bot == q_size) {
      throw std::runtime_error("internal error: scheduler queue overflow");
    }
    bot.store(local_bot, std::memory_order_relaxed);  // shared store
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return deque_surplus_unknown;
  }
  
  auto pop() -> std::pair<Fiber*, deque_surplus_result_type> {
    Fiber* result = nullptr;
    auto local_bot = bot.load(std::memory_order_relaxed);  // atomic load
    if (local_bot != 0) {
      local_bot--;
      bot.store(local_bot, std::memory_order_relaxed);  // shared store
      std::atomic_thread_fence(std::memory_order_seq_cst);
      auto f = deq[local_bot].f.load(std::memory_order_relaxed);  // atomic load
      auto old_age = age.load(std::memory_order_relaxed);      // atomic load
      if (local_bot > old_age.top) {
        result = f;
      } else {
        bot.store(0, std::memory_order_relaxed);  // shared store
        auto new_age = age_t{old_age.tag + 1, 0};
        if ((local_bot == old_age.top) &&
            age.compare_exchange_strong(old_age, new_age)) {
          result = f;
        } else {
          age.store(new_age, std::memory_order_relaxed);  // shared store
          result = nullptr;
        }
        std::atomic_thread_fence(std::memory_order_seq_cst);
      }
    }
    return std::make_pair(result, deque_surplus_unknown);
  }
  
  auto steal() -> std::pair<Fiber*, deque_surplus_result_type> {
    Fiber* result = nullptr;
    auto old_age = age.load(std::memory_order_relaxed);    // atomic load
    auto local_bot = bot.load(std::memory_order_relaxed);  // atomic load
    if (local_bot > old_age.top) {
      auto f = deq[old_age.top].f.load(std::memory_order_relaxed);  // atomic load
      auto new_age = old_age;
      new_age.top = new_age.top + 1;
      if (age.compare_exchange_strong(old_age, new_age)) {
        result = f;
      } else {
        result = nullptr;
      }
    }
    return std::make_pair(result, deque_surplus_unknown);
  }
  
};

#else

// Deque from Arora, Blumofe, and Plaxton (SPAA, 1998).
template <typename Fiber>
struct abp_deque {
  using qidx = unsigned int;
  
  // use std::atomic<age_t> for atomic access.
  // Note: Explicit alignment specifier required
  // to ensure that Clang inlines atomic loads.
  struct alignas(int64_t) age_t {
    qidx bot;
    qidx top;
  };
  
  // align to avoid false sharing
  struct alignas(64) padded_fiber {
    std::atomic<Fiber*> f;
  };
  
  static constexpr int q_size = 1000;
  std::atomic<age_t> age;
  std::array<padded_fiber, q_size> deq;
  
  abp_deque() : age(age_t{0, 0}) {}
  
  auto size(age_t a) -> unsigned int {
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
    if (next.bot == q_size) {
      throw std::runtime_error("internal error: scheduler queue overflow");
    }
    deq[orig.bot].f.store(f, std::memory_order_relaxed);
    while (true) {
      if (age.compare_exchange_strong(orig, next)) {
        break;
      }
      next.top = orig.top;
      if (size(next) == 0) {
        next.top = 0;
        next.bot = 1;
        deq[0].f.store(f, std::memory_order_relaxed);
      }
    }
    return (size(orig) == 0) ? deque_surplus_up : deque_surplus_stable;
  }
  
  auto pop() -> std::pair<Fiber*, deque_surplus_result_type> {
    auto orig = age.load();
    if (size(orig) == 0) {
      return std::make_pair(nullptr, deque_surplus_stable);
    }
    auto next = orig;
    next.bot--;
    auto f = deq[next.bot].f.load(std::memory_order_relaxed);
    assert(f != nullptr);
    while (true) {
      if (age.compare_exchange_strong(orig, next)) {
        break;
      }
      next.top = orig.top;
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
    if (! age.compare_exchange_strong(orig, next)) {
      return std::make_pair(nullptr, deque_surplus_stable);
    }
    auto r = (size(orig) == 1) ? deque_surplus_down : deque_surplus_stable;
    return std::make_pair(f, r);
  }
  
};

#endif

} // end namespace
