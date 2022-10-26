#pragma once

#include <atomic>

#include "timing.hpp"

namespace taskparts {

/*---------------------------------------------------------------------*/
/* Atomic compare-and-exchange which handles failure by having the
   caller spin wait for 4k cycles. */

// TODO: remove the code below and replace it with one of the backoff
// protocols proposed in Naama Ben-David's dissertation (see elastic.hpp)
  
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

/*---------------------------------------------------------------------*/
/* Spinning binary semaphore (for performance debugging) */

/* An implementation of semaphore that is binary (0 or 1) with spinning wait 
 * - Crashes when POSTing to a non-zero valued semaphore
 * - Crashes when WAITint on a zero     valued semaphore
 * It should be fine for use in the elastic scheduler since 
 * - Only the processor owning the semaphore will ever wait on it
 * - Every WATI is paired with exactly one POST and vice versa.
 */
class spinning_binary_semaphore {
  
   // 0  -> No processor is waiting on the semaphore
   // 1  -> A post has been issued to the semaphore
   // -1 -> A processor is waiting on the semaphore 
   std::atomic<int> flag;
  
public:
  
  spinning_binary_semaphore() : flag(0) {}
  
  ~spinning_binary_semaphore() {
    assert(flag.load() == 0); // The semaphore has to be balanced
  }

  auto post() {
    int old_value = flag++;
    assert(old_value <= 0);
  }

  auto wait() {
    int old_value = flag--;
    assert(old_value >= 0);
    while (flag.load() < 0); // Spinning
  }

};

/*---------------------------------------------------------------------*/
/* Spinning counting semaphore (for performance debugging) */

class spinning_counting_semaphore {
public:
  
  std::atomic<int32_t> count;
  
  spinning_counting_semaphore() : count(0) { }
  
  auto post() {
    count++;
  }
  
  auto wait() {
    assert(count.load() >= 0);
    auto c = --count;
    do {
      if (c >= 0) {
        break;
      }
      //cycles::spin_for(500);
      for (int i = 0; i < (1 << 6); i++) {
	_mm_pause();
      }
      c = count.load();
    } while (true);
    assert(count.load() >= 0);
  }
  
};

} // end namespace
