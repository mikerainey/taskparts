#pragma once

#include <cstddef>
#include <assert.h>

namespace taskparts {

template <class Item, size_t Capacity>
class inline_allocator {
private:
  
  mutable
  Item items[Capacity];
  
public:

  using value_type = Item;
  
  static constexpr
  size_t capacity = Capacity;
  
  value_type& operator[](size_t n) const {
    assert(n >= 0);
    assert(n < capacity);
    return items[n];
  }
  
  auto swap(inline_allocator& other) {
    value_type tmp_items[capacity];
    for (int i = 0; i < capacity; i++)
      tmp_items[i] = items[i];
    for (int i = 0; i < capacity; i++)
      items[i] = other.items[i];
    for (int i = 0; i < capacity; i++)
      other.items[i] = tmp_items[i];
  }
  
};

template <
  class Array_alloc,
  class Item_alloc = std::allocator<typename Array_alloc::value_type> >
class ringbuffer_idx {
public:
  
  using size_type = size_t;
  using value_type = typename Array_alloc::value_type;
  using allocator_type = Item_alloc;
  
  static constexpr
  int capacity = Array_alloc::capacity;
  
private:
  
  int fr;
  int sz;
  Item_alloc alloc;
  Array_alloc array;
  
public:
  
  ringbuffer_idx()
    : fr(0), sz(0) { }

  ~ringbuffer_idx() {
    clear();
  }
  
  inline
  int size() const {
    return sz;
  }
  
  inline
  bool full() const {
    return size() == capacity;
  }
  
  inline
  bool empty() const {
    return size() == 0;
  }

  inline
  void push_front(const value_type& x) {
    assert(! full());
    fr--;
    if (fr == -1)
      fr += capacity;
    sz++;
    alloc.construct(&array[fr], x);
  }
  
  inline
  void push_back(const value_type& x) {
    assert(! full());
    int bk = (fr + sz);
    if (bk >= capacity)
      bk -= capacity;
    sz++;
    alloc.construct(&array[bk], x);
  }
  
  inline
  value_type& front() const {
    assert(! empty());
    return array[fr];
  }
  
  inline
  value_type& back() const {
    assert(! empty());
    int bk = fr + sz - 1;
    if (bk >= capacity)
      bk -= capacity;
    return array[bk];
  }
  
  inline
  value_type pop_front() {
    assert(! empty());
    value_type v = front();
    alloc.destroy(&(front()));
    fr++;
    if (fr == capacity)
      fr = 0;
    sz--;
    return v;
  }
  
  inline
  value_type pop_back() {
    assert(! empty());
    value_type v = back();
    alloc.destroy(&(back()));
    sz--;
    return v;
  }

  void clear() {
    while (! empty()) {
      pop_back();
    }
  }
  
};

static constexpr
int dflt_buffer_capacity = (1 << 10);

template <class Item, int capacity=dflt_buffer_capacity>
using ringbuffer = ringbuffer_idx<inline_allocator<Item, capacity>>;

} // end namespace
