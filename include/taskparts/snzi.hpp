#pragma once

#include <new>

#include "atomic.hpp"
#include "perworker.hpp"

namespace taskparts {
  
/*---------------------------------------------------------------------*/
/* Scalable Non-Zero Indicator methods */

static constexpr
int snzi_one_half = -1;

template <typename Snzi_Node>
auto snzi_increment(Snzi_Node& node) {
  auto& X = node.get_counter();
  auto parent = node.get_parent();
  bool succ = false;
  int undo_arr = 0;
  while (! succ) {
    auto x = X.load();
    if (x.c >= 1) {
      auto orig = x;
      auto next = x;
      next.c++;
      next.v++;
      succ = compare_exchange_with_backoff(X, orig, next);
    }
    if (x.c == 0) {
      auto orig = x;
      auto next = x;
      next.c = snzi_one_half;
      next.v++;
      if (compare_exchange_with_backoff(X, orig, next)) {
        succ = true;
        x.c = snzi_one_half;
        x.v++;
      }
    }
    if (x.c == snzi_one_half) {
      if (! Snzi_Node::is_root_node(parent)) {
        parent->increment();
      }
      auto orig = x;
      auto next = x;
      next.c = 1;
      if (! compare_exchange_with_backoff(X, orig, next)) {
        undo_arr++;
      }
    }
  }
  if (Snzi_Node::is_root_node(parent)) {
    return;
  }
  while (undo_arr > 0) {
    parent->decrement();
    undo_arr--;
  }
}

template <typename Snzi_Node>
auto snzi_decrement(Snzi_Node& node) -> bool {
  auto& X = node.get_counter();
  auto parent = node.get_parent();
  while (true) {
    auto x = X.load();
    assert(x.c >= 1);
    auto orig = x;
    auto next = x;
    next.c--;
    if (compare_exchange_with_backoff(X, orig, next)) {
      bool s = (x.c == 1);
      if (Snzi_Node::is_root_node(parent)) {
        return s;
      } else if (s) {
        return parent->decrement();
      } else {
        return false;
      }
    }
  }
}

/*---------------------------------------------------------------------*/
/* Scalable Non-Zero Indicator tree node */
  
class snzi_node {
public:

  using counter_type = struct counter_struct {
    int c; // counter value
    int v; // version number
  };

private:
  
  alignas(TASKPARTS_CACHE_LINE_SZB)
  std::atomic<counter_type> counter;

  alignas(TASKPARTS_CACHE_LINE_SZB)
  snzi_node* parent;
  
public:

  snzi_node(snzi_node* _parent = nullptr) {
    {
      parent = _parent;
    }
    {
      counter_type c = {.c = 0, .v = 0};
      counter.store(c);
    }
  }

  auto get_counter() -> std::atomic<counter_type>& {
    return counter;
  }

  auto get_parent() -> snzi_node* {
    return parent;
  }

  auto increment() {
    snzi_increment(*this);
  }

  auto decrement() -> bool {
    return snzi_decrement(*this);
  }

  auto is_nonzero() -> bool {
    return get_counter().load().c > 0;
  }

  static
  auto is_root_node(const snzi_node* n) -> bool {
    return n == nullptr;
  }
  
};

/*---------------------------------------------------------------------*/
/* Scalable Non-Zero Indicator tree container */

template <size_t height=perworker::default_max_nb_workers_lg>
class snzi_fixed_capacity_tree {
public:

  using node_type = snzi_node;
  
private:

  static constexpr
  int nb_leaves = 1 << height;
  
  static constexpr
  int heap_size = 2 * nb_leaves;

  cache_aligned_fixed_capacity_array<node_type, heap_size> heap;

  alignas(TASKPARTS_CACHE_LINE_SZB)
  node_type root;

  auto initialize_heap() {
    new (&root) node_type;
    // cells at indices 0 and 1 are not used
    for (size_t i = 2; i < 4; i++) {
      new (&heap[i]) node_type(&(root));
    }
    for (size_t i = 4; i < heap_size; i++) {
      new (&heap[i]) node_type(&heap[i / 2]);
    }
  }

  auto destroy_heap() {
    for (size_t i = 2; i < heap_size; i++) {
      (&heap[i])->~node_type();
    }
  }
  
  inline
  auto leaf_position_of(size_t i) -> size_t {
    auto k = nb_leaves + (i & (nb_leaves - 1));
    assert(k >= 2 && k < heap_size);
    return k;
  }

  inline
  auto at(size_t i) -> node_type& {
    assert(i < nb_leaves);
    return heap[leaf_position_of(i)];
  }

public:

  snzi_fixed_capacity_tree() {
    initialize_heap();
  }

  ~snzi_fixed_capacity_tree() {
    destroy_heap();
  }

  inline
  auto operator[](size_t i) -> node_type& {
    return at(i);
  }

  inline
  auto mine() -> node_type& {
    return at(perworker::my_id());
  }

  auto get_root() -> node_type& {
    return root;
  }
  
};

/*---------------------------------------------------------------------*/
/* SNZI-based, termination detection */

template <size_t height=perworker::default_max_nb_workers_lg>
class snzi_termination_detection {
private:

  snzi_fixed_capacity_tree<height> tree;

public:

  auto set_active(bool active) -> bool {
    if (active) {
      tree.mine().increment();
      return false;
    } else {
      return tree.mine().decrement();
    }
  }

  auto is_terminated() -> bool {
    return ! tree.get_root().is_nonzero();
  }
  
};

} // end namespace
