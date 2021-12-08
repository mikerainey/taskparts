#pragma once

#include <atomic>

namespace taskparts {

class spinlock {
  std::atomic_flag locked = ATOMIC_FLAG_INIT ;
public:
  void lock()     { while (locked.test_and_set(std::memory_order_acquire)) { ; } }
  bool try_lock() { lock(); return true; }
  void unlock()   { locked.clear(std::memory_order_release); }
};

} // end namespace
