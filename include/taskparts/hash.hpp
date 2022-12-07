#pragma once

#include <cstdint>
#include <cassert>

#include "perworker.hpp"

namespace taskparts {
  
inline
uint64_t hash(uint64_t u) {
  uint64_t v = u * 3935559000370003845ul + 2691343689449507681ul;
  v ^= v >> 21;
  v ^= v << 37;
  v ^= v >>  4;
  v *= 4768777513237032717ul;
  v ^= v << 20;
  v ^= v >> 41;
  v ^= v <<  5;
  return v;
}

perworker::array<uint64_t> rng;

inline
auto random_number(size_t my_id = perworker::my_id()) -> uint64_t {
  auto& nb = rng[my_id];
  nb = hash(my_id) + hash(nb);
  return nb;
}

inline
auto random_other_worker(size_t my_id) -> size_t {
  auto nb_workers = perworker::nb_workers();
  assert(nb_workers > 1);
  size_t id = (size_t)(random_number(my_id) % (nb_workers - 1));
  if (id >= my_id) {
    id++;
  }
  assert(id != my_id);
  assert(id >= 0 && id < nb_workers);
  return id;
}
  
} // end namespace
