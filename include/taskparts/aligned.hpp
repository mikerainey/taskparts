#pragma once

#ifndef TASKPARTS_CACHE_LINE_SZB
#define TASKPARTS_CACHE_LINE_SZB 128
#endif

#if defined(TASKPARTS_POSIX)
#include "posix/aligned.hpp"
#elif defined(TASKPARTS_DARWIN)
#include "darwin/aligned.hpp"
#elif defined (TASKPARTS_NAUTILUS)
#include "nautilus/aligned.hpp"
#else
#error need to declare platform (e.g., TASKPARTS_POSIX)
#endif

#include <cstddef>

namespace taskparts {

/*---------------------------------------------------------------------*/
/* Cache-aligned, fixed-capacity array */

/* This class provides storage for a given number, capacity, of items
 * of given type, Item, also ensuring that the starting of address of
 * each item is aligned by a multiple of a given number of bytes,
 * cache_align_szb (defaultly, TASKPARTS_CACHE_LINE_SZB).
 *
 * The class *does not* itself initialize (or deinitialize) the
 * storage cells.
 */
template <typename Item, size_t capacity,
          size_t cache_align_szb=TASKPARTS_CACHE_LINE_SZB>
class cache_aligned_fixed_capacity_array {
private:
  
  static constexpr
  int item_szb = sizeof(Item);
  
  using aligned_item_type =
    typename std::aligned_storage<item_szb, cache_align_szb>::type;
  
  aligned_item_type items[capacity] __attribute__ ((aligned (cache_align_szb)));
  
public:

  inline
  Item& at(size_t i) {
    return *reinterpret_cast<Item*>(items + i);
  }

  Item& operator[](size_t i) {
    assert(i < capacity);
    return at(i);
  }
  
  size_t size() const {
    return capacity;
  }

};

/*---------------------------------------------------------------------*/
/* Cache-aligned malloc */

template <typename Item,
          size_t cache_align_szb=TASKPARTS_CACHE_LINE_SZB>
auto aligned_alloc_uninitialized_array(size_t size) -> Item* {
  return (Item*)aligned_alloc<cache_align_szb>(sizeof(Item) * size);
}

template <typename Item,
          size_t cache_align_szb=TASKPARTS_CACHE_LINE_SZB>
auto aligned_alloc_uninitialized() -> Item* {
  return aligned_alloc_uninitialized_array<Item, cache_align_szb>(1);
}

template <typename Item>
class Malloc_deleter {
public:
  void operator()(Item* ptr) {
    std::free(ptr);
  }
};

/*---------------------------------------------------------------------*/
/* Cache-aligned (heap-allocated) array */

/* This class provides storage for a given number, size, of items
 * of given type, Item, also ensuring that the starting of address of
 * each item is aligned by a multiple of a given number of bytes,
 * cache_align_szb (defaultly, TASKPARTS_CACHE_LINE_SZB).
 *
 * The class *does not* itself initialize (or deinitialize) the
 * storage cells.
 */
template <typename Item,
          size_t cache_align_szb=TASKPARTS_CACHE_LINE_SZB>
class cache_aligned_array {
private:
  
  static constexpr
  int item_szb = sizeof(Item);
  
  using aligned_item_type =
    typename std::aligned_storage<item_szb, cache_align_szb>::type;

  std::unique_ptr<aligned_item_type, Malloc_deleter<aligned_item_type>> items;

  size_t _size;

  inline
  Item& at(size_t i) {
    assert(i < size());
    return *reinterpret_cast<Item*>(items.get() + i);
  }
  
public:

  cache_aligned_array(size_t __size)
    : _size(__size) {
    items.reset(aligned_alloc_uninitialized_array<aligned_item_type>(__size));
  }
  
  Item& operator[](size_t i) {
    return at(i);
  }
  
  size_t size() const {
    return _size;
  }

  // Iterator

  using value_type = Item;
  using iterator = value_type*;    

  auto begin() -> iterator {
    return reinterpret_cast<Item*>(items.get());
  }

  auto end() -> iterator {
    return reinterpret_cast<Item*>(items.get() + size());
  }

};
  
} // end namespace

