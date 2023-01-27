#include <algorithm>
#include <cstdio>
#include <cstdint>
#include "rollforward.h"

#define SUB(array, row_sz, i, j) (array[i * row_sz + j])
#define INF           INT_MAX-1
#define D 32

extern
void to_loop_handler(int *dist, int vertices, int via_lo, int via_hi,
                     int from_lo, int from_hi, int to_lo, int to_hi,
                     bool &promoted);

void rollforward_handler_annotation __rf_handle_from_loop_handler(int* dist, int vertices, int via_lo, int via_hi, int from_lo, int from_hi, bool& promoted) {
  to_loop_handler(dist, vertices, via_lo, via_hi, from_lo, from_hi, 0, 0, promoted);
  rollbackward
}

void rollforward_handler_annotation __rf_handle_from_from_loop_handler(int* dist, int vertices, int via_lo, int via_hi, int from_lo, int from_hi, bool& promoted) {
  to_loop_handler(dist, vertices, via_lo, via_hi, from_lo, from_hi, 0, 0, promoted);
  rollbackward
}

void rollforward_handler_annotation __rf_handle_to_to_loop_handler(int* dist, int vertices, int via_lo, int via_hi, int from_lo, int from_hi, int to_lo, int to_hi, bool& promoted) {
  to_loop_handler(dist, vertices, via_lo, via_hi, from_lo, from_hi, to_lo, to_hi, promoted);
  rollbackward
}

void rollforward_handler_annotation __rf_handle_from_to_loop_handler(int* dist, int vertices, int via_lo, int via_hi, int from_lo, int from_hi, int to_lo, int to_hi, bool& promoted) {
  to_loop_handler(dist, vertices, via_lo, via_hi, from_lo, from_hi, to_lo, to_hi, promoted);
  rollbackward
}

void floyd_warshall_interrupt(int* dist, int vertices, int via_lo, int via_hi) {
  for(; via_lo < via_hi; via_lo++) {
    int from_lo = 0;
    int from_hi = vertices;
    if (! (from_lo < from_hi)) {
      return;
    }
    for (; ; ) {
      int to_lo = 0;
      int to_hi = vertices;
      if (! (to_lo < to_hi)) {
        return;
      }
      for (; ; ) {
        int to_lo2 = to_lo;
        int to_hi2 = std::min(to_lo + D, to_hi);
        for(; to_lo2 < to_hi2; to_lo2++) {
          if ((from_lo != to_lo2) && (from_lo != via_lo) && (to_lo2 != via_lo)) {
            SUB(dist, vertices, from_lo, to_lo2) = 
              std::min(SUB(dist, vertices, from_lo, to_lo2), 
                       SUB(dist, vertices, from_lo, via_lo) + SUB(dist, vertices, via_lo, to_lo2));
          }
        }
        to_lo = to_lo2;
        if (! (to_lo < to_hi)) {
          break;
        }
	bool promoted = false;
	__rf_handle_from_to_loop_handler(dist, vertices, via_lo, via_hi, from_lo, from_hi, to_lo, to_hi, promoted);
	if (rollforward_branch_unlikely(promoted)) {
	  return;
	}
      }
      from_lo++;
      if (! (from_lo < from_hi)) {
        break;
      }
      bool promoted = false;
      __rf_handle_from_loop_handler(dist, vertices, via_lo, via_hi, from_lo, from_hi, promoted);
      if (rollforward_branch_unlikely(promoted)) {
	return;
      }
    }
  }
}

void floyd_warshall_interrupt_from(int* dist, int vertices, int via_lo, int via_hi, int from_lo, int from_hi) {
  if (! (from_lo < from_hi)) {
    return;
  }
  for (; ; ) {
    int to_lo = 0;
    int to_hi = vertices;
    if (! (to_lo < to_hi)) {
      return;
    }
    for (; ; ) {
      int to_lo2 = to_lo;
      int to_hi2 = std::min(to_lo + D, to_hi);
      for(; to_lo2 < to_hi2; to_lo2++) {
        if ((from_lo != to_lo2) && (from_lo != via_lo) && (to_lo2 != via_lo)) {
          SUB(dist, vertices, from_lo, to_lo2) = 
            std::min(SUB(dist, vertices, from_lo, to_lo2), 
                     SUB(dist, vertices, from_lo, via_lo) + SUB(dist, vertices, via_lo, to_lo2));
        }
      }
      to_lo = to_lo2;
      if (! (to_lo < to_hi)) {
        break;
      }
      bool promoted = false;
      __rf_handle_from_to_loop_handler(dist, vertices, via_lo, via_hi, from_lo, from_hi, to_lo, to_hi, promoted);
      if (rollforward_branch_unlikely(promoted)) {
	return;
      }
    }
    from_lo++;
    if (! (from_lo < from_hi)) {
      break;
    }
    bool promoted = false;
    __rf_handle_from_from_loop_handler(dist, vertices, via_lo, via_hi, from_lo, from_hi, promoted);
    if (rollforward_branch_unlikely(promoted)) {
      return;
    }
  }
}

void floyd_warshall_interrupt_to(int* dist, int vertices, int via_lo, int via_hi, int from_lo, int from_hi, int to_lo, int to_hi) {
  if (! (to_lo < to_hi)) {
    return;
  }
  for (; ; ) {
    int to_lo2 = to_lo;
    int to_hi2 = std::min(to_lo + D, to_hi);
    for(; to_lo2 < to_hi2; to_lo2++) {
      if ((from_lo != to_lo2) && (from_lo != via_lo) && (to_lo2 != via_lo)) {
        SUB(dist, vertices, from_lo, to_lo2) = 
          std::min(SUB(dist, vertices, from_lo, to_lo2), 
                   SUB(dist, vertices, from_lo, via_lo) + SUB(dist, vertices, via_lo, to_lo2));
      }
    }
    to_lo = to_lo2;
    if (! (to_lo < to_hi)) {
      break;
    }
    bool promoted = false;
    __rf_handle_to_to_loop_handler(dist, vertices, via_lo, via_hi, from_lo, from_hi, to_lo, to_hi, promoted);
    if (rollforward_branch_unlikely(promoted)) {
      return;
    }
  }
}
