#ifndef TASKPARTS_TPALRTS
#error "need to compile with tpal flags, e.g., TASKPARTS_TPALRTS"
#endif
#include <taskparts/benchmark.hpp>
#include <algorithm>
#include <cassert>

#define SUB(array, row_sz, i, j) (array[i * row_sz + j])

#define INF           INT_MAX-1
#define D 32



/* Handler functions */
/* ================= */

extern
void floyd_warshall_interrupt(int* dist, int vertices, int via_lo, int via_hi);

extern
void floyd_warshall_interrupt_from(int* dist, int vertices, int via_lo, int via_hi, int from_lo, int from_hi);

extern
void floyd_warshall_interrupt_to(int* dist, int vertices, int via_lo, int via_hi, int from_lo, int from_hi, int to_lo, int to_hi);

void to_loop_handler(int* dist, int vertices, int via_lo, int via_hi, int from_lo, int from_hi, int to_lo, int to_hi, bool& promoted) {
  if ((via_hi - via_lo) <= 0) {
    promoted = false; return;
  }
  auto nb_from = from_hi - from_lo;
  if (nb_from == 0) {
    promoted = false; return;
  }
  auto via_hi2 = via_lo + 1;
  assert((via_hi2 - via_lo) == 1);
  assert((via_hi - via_hi2) >= 0);
  if (nb_from == 1) {
    auto nb_to = to_lo - to_hi;
    if (nb_to <= 1) {
      promoted = false; return;
    }
    assert(nb_to >= 2);
    auto to_mid = (to_lo + to_hi) / 2;
    tpalrts_promote_via_nativefj([=] {
      floyd_warshall_interrupt_to(dist, vertices, via_lo, via_hi2, from_lo, from_hi, to_lo, to_mid);
    }, [=] {
      floyd_warshall_interrupt_to(dist, vertices, via_lo, via_hi2, from_lo, from_hi, to_mid, to_hi);
    }, [=] {
      floyd_warshall_interrupt(dist, vertices, via_hi2, via_hi);
    }, taskparts::bench_scheduler());
    promoted = true; return;
  }
  assert(nb_from >= 2);
  auto from_mid = (from_lo + from_hi) / 2;
  tpalrts_promote_via_nativefj([=] {
    tpalrts_promote_via_nativefj([=] {
      floyd_warshall_interrupt_to(dist, vertices, via_lo, via_hi2, from_lo, from_hi, to_lo, to_hi);
    }, [=] {
      floyd_warshall_interrupt_from(dist, vertices, via_lo, via_hi2, from_lo, from_mid);
    }, [=] { }, taskparts::bench_scheduler());
  }, [=] {
    floyd_warshall_interrupt_from(dist, vertices, via_lo, via_hi2, from_mid, from_hi);
  }, [=] {
    floyd_warshall_interrupt(dist, vertices, via_hi2, via_hi);
  }, taskparts::bench_scheduler());
  promoted = true;
}

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

/* Outlined-loop functions */
/* ======================= */

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

int vertices = 3000;
int* dist = nullptr;

auto init_input(int vertices) {
  int* dist = (int*)malloc(sizeof(int) * vertices * vertices);
  for(int i = 0; i < vertices; i++) {
    for(int j = 0; j < vertices; j++) {
      SUB(dist, vertices, i, j) = ((i == j) ? 0 : INF);
    }
  }
  for (int i = 0 ; i < vertices ; i++) {
    for (int j = 0 ; j< vertices; j++) {
      if (i == j) {
	SUB(dist, vertices, i, j) = 0;
      } else {
	int num = i + j;
	if (num % 3 == 0) {
	  SUB(dist, vertices, i, j) = num / 2;
	} else if (num % 2 == 0) {
	  SUB(dist, vertices, i, j) = num * 2;
	} else {
	  SUB(dist, vertices, i, j) = num;
	}
      }
    }
  }
  return dist;
};


int main() {
  
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    floyd_warshall_interrupt(dist, vertices, 0, vertices);
  }, [&] (auto sched) {
    dist = init_input(vertices);
  }, [&] (auto sched) {
#if ! defined(NDEBUG) && defined(TASKPARTS_LINUX)
    bool check = deepsea::cmdline::parse_or_default_bool("check", false);
    if (check) {
      auto dist2 = init_input(vertices);
      floyd_warshall_serial(dist2, vertices);
      int nb_diffs = 0;
      for (int i = 0; i < vertices; i++) {
	for (int j = 0; j < vertices; j++) {
	  if (SUB(dist, vertices, i, j) != SUB(dist2, vertices, i, j)) {
	    nb_diffs++;
	  }
	}
      }
      printf("nb_diffs %d\n", nb_diffs);
      free(dist2);
    }
#endif
    free(dist);
  });

  return 0;
}
