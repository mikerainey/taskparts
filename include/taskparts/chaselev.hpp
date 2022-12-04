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

/*---------------------------------------------------------------------*/
/* Chase-Lev Work-Stealing Deque data structure
 * 
 * This implementation is based on
 *   https://gist.github.com/Amanieu/7347121
 */
  
template <typename Fiber>
class chaselev {

  using index_type = long;
  
  class circular_array {
  private:
    
    cache_aligned_array<std::atomic<Fiber*>> items;
    
    std::unique_ptr<circular_array> previous;

  public:
    
    circular_array(index_type n) : items(n) {}
    
    index_type size() const {
      return items.size();
    }
    
    auto get(index_type index) -> Fiber* {
      return items[index % size()].load(std::memory_order_relaxed);
    }
    
    auto put(index_type index, Fiber* x) {
      items[index % size()].store(x, std::memory_order_relaxed);
    }
    
    auto grow(index_type top, index_type bottom) -> circular_array* {
      circular_array* new_array = new circular_array(size() * 2);
      new_array->previous.reset(this);
      for (index_type i = top; i != bottom; ++i) {
        new_array->put(i, get(i));
      }
      return new_array;
    }

  };

  std::atomic<circular_array*> array;
  
  std::atomic<index_type> top, bottom;

public:
  
  chaselev()
    : array(new circular_array(64)), top(0), bottom(0) { }
  
  ~chaselev() {
    circular_array* p = array.load(std::memory_order_relaxed);
    if (p) {
      delete p;
    }
  }

  auto size() -> index_type {
    auto b = bottom.load(std::memory_order_relaxed);
    auto t = top.load(std::memory_order_relaxed);
    return b - t;
  }

  auto empty() -> bool {
    return size() == 0;
  }

  auto push(Fiber* x) -> deque_surplus_result_type {
    auto b = bottom.load(std::memory_order_relaxed);
    auto t = top.load(std::memory_order_acquire);
    circular_array* a = array.load(std::memory_order_relaxed);
    if (b - t > a->size() - 1) {
      a = a->grow(t, b);
      array.store(a, std::memory_order_relaxed);
    }
    a->put(b, x);
    std::atomic_thread_fence(std::memory_order_release);
    bottom.store(b + 1, std::memory_order_relaxed);
    return deque_surplus_unknown;
  }

  auto pop() -> std::pair<Fiber*, deque_surplus_result_type> {
    auto b = bottom.load(std::memory_order_relaxed) - 1;
    circular_array* a = array.load(std::memory_order_relaxed);
    bottom.store(b, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    auto t = top.load(std::memory_order_relaxed);
    if (t <= b) {
      auto x = a->get(b);
      if (t == b) {
        if (!top.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
          x = nullptr;
        }
        bottom.store(b + 1, std::memory_order_relaxed);
      }
      return std::make_pair(x, deque_surplus_unknown);
    } else {
      bottom.store(b + 1, std::memory_order_relaxed);
      return std::make_pair(nullptr, deque_surplus_unknown);
    }
  }

  auto steal() -> std::pair<Fiber*, deque_surplus_result_type> {
    auto t = top.load(std::memory_order_acquire);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    auto b = bottom.load(std::memory_order_acquire);
    Fiber* x = nullptr;
    if (t < b) {
      circular_array* a = array.load(std::memory_order_relaxed);
      x = a->get(t);
      if (!top.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed)) {
        return std::make_pair(nullptr, deque_surplus_unknown);
      }
    }
    return std::make_pair(x, deque_surplus_unknown);
  }
  
};

} // end namespace
