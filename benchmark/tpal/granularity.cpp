#include "taskparts/nativeforkjoin.hpp"
#include <cstdint>
#ifndef TASKPARTS_TPALRTS
#error "need to compile with tpal flags, e.g., TASKPARTS_TPALRTS"
#endif
#include <taskparts/benchmark.hpp>

extern
void nb_occurrences_heartbeat(void* lo, void* hi, uint64_t r, int64_t* dst);

void nb_occurrences_handler(void* lo, void* hi, uint64_t r, int64_t* dst, bool& promoted) {
  auto n = (char*)hi - (char*)lo;
  if (n <= 1) {
    promoted = false;
  } else {
    int64_t dst1, dst2;
    auto mid = (void*)((char*)lo + (n / 2));
    auto f1 = [&] {
      nb_occurrences_heartbeat(lo, mid, 0, &dst1);
    };
    auto f2 = [&] {
      nb_occurrences_heartbeat(mid, hi, 0, &dst2);
    };
    auto fj = [&] {
      *dst = r + dst1 + dst2;
    };
    tpalrts_promote_via_nativefj(f1, f2, fj, taskparts::bench_scheduler());
    promoted = true;
  }
}


extern    
auto select_item_type() -> void ;

int main() {
  select_item_type();
}
