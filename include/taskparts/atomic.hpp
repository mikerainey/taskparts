#pragma once

#include <atomic>

#include "timing.hpp"

namespace taskparts {

template <class T>
auto compare_exchange_with_backoff(std::atomic<T>& cell, T& expected, T desired) -> bool {
  static constexpr
  int backoff_nb_cycles = 1l << 12;
  if (cell.compare_exchange_strong(expected, desired)) {
    return true;
  }
  cycles::spin_for(backoff_nb_cycles);
  return false;
}

static inline
auto busywait_pause() {
#if defined(TASKPARTS_X64)
  __builtin_ia32_pause();
#else
#error need to declare platform (e.g., TASKPARTS_X64)
#endif
}
  
} // end namespace
