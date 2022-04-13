#pragma once

#include <new>
#include <assert.h>

#include "atomic.hpp"
#include "perworker.hpp"

namespace taskparts {
  
/*---------------------------------------------------------------------*/
/* Combining tree */

class combnode {
public:
  alignas(TASKPARTS_CACHE_LINE_SZB)
  std::atomic<int64_t> counter;
}; 

class combtree {
public:

  using index_type = size_t;

private:

  // we use level order indexing of tree nodes
  
  size_t height;
  
  combnode* heap;

  static
  auto heap_size_of(size_t height) -> size_t {
    return (1 << (height + 1)) - 1;
  }

  auto get_heap_size() -> size_t {
    return heap_size_of(height);
  }

  auto get_nb_leaves() -> size_t {
    return 1 << height;
  }

  auto get_first_leaf() -> size_t {
    return heap_size_of(height - 1);
  }    

  auto child_of(index_type n, int d) -> index_type {
    assert((d == 1) || (d == 2));
    assert(n < get_heap_size());
    auto c = n * 2 + d;
    assert(c < get_heap_size());
    return c;
  }

  auto is_root(index_type n) -> bool {
    return n == 0;
  }

  auto increment_counter_rec(index_type n, int64_t i) -> void {
    assert(n < get_heap_size());
    heap[n].counter.fetch_add(i);
    if (!is_root(n)) {
      increment_counter_rec(parent_of(n), i);
    }
  }

public:

  combtree(size_t height)
    : height(height) {
    heap = new combnode[get_heap_size()];
  }

  ~combtree() {
    delete [] heap;
    heap = nullptr;
  }

  auto parent_of(index_type n) -> index_type {
    assert(! is_root(n));
    return (n - 1) / 2;
  }

  auto left_child_of(index_type n) -> index_type {
    return child_of(n, 1);
  }

  auto right_child_of(index_type n) -> index_type {
    return child_of(n, 2);
  }

  auto is_leaf(index_type n) -> bool {
    auto f = get_first_leaf();
    assert(n < get_heap_size());
    return f >= get_first_leaf();
  }

  auto increment_counter(index_type n, int64_t i) {
    increment_counter_rec(n, i);
  }

  auto increment_leaf_counter(index_type l, int64_t i) {
    if (l >= get_nb_leaves()) {
      printf("l=%lu n=%lu\n",l, get_nb_leaves());
    }
    assert(l < get_nb_leaves());
    increment_counter(get_first_leaf() + l, i);
  }

  auto read_counter(index_type n) -> int64_t {
    assert(n < get_heap_size());
    return heap[n].counter.load();
  }
  
};

} // end namespace
