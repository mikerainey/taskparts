#pragma once

#include <cstdint>

namespace taskparts {
  
using hash_value_type = uint64_t;

inline
hash_value_type hash(hash_value_type u) {
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

} // end namespace
