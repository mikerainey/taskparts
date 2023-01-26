#include "taskparts/nativeforkjoin.hpp"
#include <cstdint>
#ifndef TASKPARTS_TPALRTS
#error "need to compile with tpal flags, e.g., TASKPARTS_TPALRTS"
#endif
#include <taskparts/benchmark.hpp>

extern
void nb_occurrences_heartbeat(void* b, uint64_t lo, uint64_t hi, uint64_t r, uint64_t* dst);

void nb_occurrences_handler(void* b, uint64_t lo, uint64_t hi, uint64_t r, uint64_t* dst, bool& promoted) {
  if ((hi - lo) <= 1) {
    promoted = false;
  } else {
    uint64_t dst1, dst2;
    auto mid = (lo + hi) / 2;
    auto f1 = [&] {
      nb_occurrences_heartbeat(b, lo, mid, 0, &dst1);
    };
    auto f2 = [&] {
      nb_occurrences_heartbeat(b, mid, hi, 0, &dst2);
    };
    auto fj = [&] {
      *dst = r + dst1 + dst2;
    };
    tpalrts_promote_via_nativefj(f1, f2, fj, taskparts::bench_scheduler());
    promoted = true;
  }
}


extern    
auto select_item_type();

int main() {
  select_item_type();
}
