#pragma once

#include <atomic>

#include "timing.hpp"

namespace taskparts {

template <typename T, typename Update>
static
auto update_atomic(std::atomic<T>& c, const Update& u,
                   size_t my_id = perworker::my_id()) -> T {
#ifndef TASKPARTS_ELASTIC_OVERRIDE_ADAPTIVE_BACKOFF
  while (true) {
    auto v = c.load();
    auto orig = v;
    auto next = u(orig);
    if (c.compare_exchange_strong(orig, next)) {
      return next;
    }
  }
#else
  // We mitigate contention on the atomic cell by using Adaptive
  // Feedback (as proposed in Ben-David's dissertation)
  uint64_t max_delay = 1;
  while (true) {
    auto v = c.load();
    auto orig = v;
    auto next = u(orig);
    if (c.compare_exchange_strong(orig, next)) {
      return next;
    }
    max_delay *= 2;
    cycles::spin_for(hash(my_id + max_delay) % max_delay);
  }
#endif
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
      cycles::spin_for(2000);
      /*
      for (int i = 0; i < (1 << 6); i++) {
	_mm_pause();
      }
      */
      c = count.load();
    } while (true);
    assert(count.load() >= 0);
  }
  
};

} // end namespace
