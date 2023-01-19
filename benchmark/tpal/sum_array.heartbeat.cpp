#include <algorithm>
#include <cstdio>
#include "rollforward.h"

#define D 64

extern
void sum_array_handler(double* a, uint64_t lo, uint64_t hi, double r, double* dst, bool& promoted);    

void rollforward_handler_annotation __rf_handle_sum_array_heartbeat(double* a, uint64_t lo, uint64_t hi,
								    double r, double* dst, bool& promoted) {
  sum_array_handler(a, lo, hi, r, dst, promoted);
  rollbackward
}

void sum_array_heartbeat(double* a, uint64_t lo, uint64_t hi, double r, double* dst) {
  if (! (lo < hi)) {
    goto exit;
  }
  for (; ; ) {
    uint64_t lo2 = lo;
    uint64_t hi2 = std::min(lo + D, hi);
    for (; lo2 < hi2; lo2++) {
      r += a[lo2];
    }
    lo = lo2;
    if (! (lo < hi)) {
      break;
    }
    bool promoted = false;
    __rf_handle_sum_array_heartbeat(a, lo, hi, r, dst, promoted);
    if (rollforward_branch_unlikely(promoted)) {
      return;
    }
  }
 exit:
  *dst = r;
}
