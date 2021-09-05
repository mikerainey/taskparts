#pragma once

#if defined(TASKPARTS_POSIX)
#include "posix/perworkerid.hpp"
#elif defined (TASKPARTS_NAUTILUS)
#include "nautilus/perworkerid.hpp"
#else
#error need to declare platform (e.g., TASKPARTS_POSIX)
#endif
#include "aligned.hpp"

/*---------------------------------------------------------------------*/
/* Per-worker array */

namespace taskparts {
namespace perworker {

template <typename Item, size_t capacity=default_max_nb_workers>
class array {
private:

  cache_aligned_fixed_capacity_array<Item, capacity> items;

public:
  
  using value_type = Item;
  using reference = Item&;

  array() {
    for (size_t i = 0; i < items.size(); ++i) {
      new (&items[i]) value_type;
    }
  }

  ~array() {
    for (size_t i = 0; i < items.size(); ++i) {
      items[i].~value_type();
    }
  }

  size_t size() const {
    return capacity;
  }

  reference operator[](size_t i) {
    assert(i < capacity);
    return items[i];
  }

  auto mine() -> reference {
    return items[id::get_my_id()];
  }
  
};

} // end namespace
} // end namespace
