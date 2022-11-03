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

// Deque from Arora, Blumofe, and Plaxton (SPAA, 1998).
template <typename Fiber>
struct abp {
  using qidx = unsigned int;
  using tag_t = unsigned int;
  
  // use std::atomic<age_t> for atomic access.
  // Note: Explicit alignment specifier required
  // to ensure that Clang inlines atomic loads.
  struct alignas(int64_t) age_t {
    tag_t tag;
    qidx top;
  };
  
  // align to avoid false sharing
  struct alignas(64) padded_fiber {
    std::atomic<Fiber*> f;
  };
  
  static constexpr int q_size = 1000;
  std::atomic<qidx> bot;
  std::atomic<age_t> age;
  std::array<padded_fiber, q_size> deq;
  
  abp() : bot(0), age(age_t{0, 0}) {}
  
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

} // end namespace
