#pragma once

//  -DNDEBUG -O3 -march=znver2 -fno-verbose-asm 

#include <stdint.h>
#include <algorithm>

double sum_array_serial(double* a, uint64_t lo, uint64_t hi) {
  double r = 0.0;
  for (uint64_t i = lo; i != hi; i++) {
    r += a[i];
  }
  return r;
}

#define unlikely(x)    __builtin_expect(!!(x), 0)

#define D 64

extern volatile
int heartbeat;

extern
int sum_array_heartbeat_handler(double* a, uint64_t lo, uint64_t hi, double r, double* dst);

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
    if (unlikely(heartbeat)) { 
      if (sum_array_heartbeat_handler(a, lo, hi, r, dst)) {
        return;
      }
    }
  }
 exit:
  *dst = r;
}
