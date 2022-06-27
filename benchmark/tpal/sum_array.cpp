#ifndef TASKPARTS_TPALRTS
#error "need to compile with tpal flags, e.g., TASKPARTS_TPALRTS"
#endif
#include <taskparts/benchmark.hpp>

void sum_array_heartbeat(double* a, uint64_t lo, uint64_t hi, double r, double* dst);

void __attribute__((preserve_all, noinline)) __rf_handle_sum_array_heartbeat(double* a, uint64_t lo, uint64_t hi, double r, double* dst, bool& promoted) {
  if ((hi - lo) <= 1) {
    promoted = false;
  } else {
    double dst1, dst2;
    auto mid = (lo + hi) / 2;
    auto f1 = [&] {
      sum_array_heartbeat(a, lo, mid, 0.0, &dst1);
    };
    auto f2 = [&] {
      sum_array_heartbeat(a, mid, hi, 0.0, &dst2);
    };
    auto fj = [&] {
      *dst = r + dst1 + dst2;
    };
    tpalrts_promote_via_nativefj(f1, f2, fj, taskparts::bench_scheduler());
    promoted = true;
  }
  void* ra_dst = __builtin_return_address(0);
  void* ra_src = NULL;
  // Binary search over the rollbackwards
  {
    int64_t i = 0, j = (int64_t)rollforward_table_size - 1;
    int64_t k;
    while (i <= j) {
      k = i + ((j - i) / 2);
      if ((uint64_t)rollback_table[k].from == (uint64_t)ra_dst) {
	ra_src = rollback_table[k].to;
	break;
      } else if ((uint64_t)rollback_table[k].from < (uint64_t)ra_dst) {
	i = k + 1;
      } else {
	j = k - 1;
      }
    }
  }
  if (ra_src != NULL) {
    void* fa = __builtin_frame_address(0);
    void** rap = (void**)((char*)fa + 8);
    *rap = ra_src;
  } else {
    printf("oops! %lx %lu\n\n",ra_dst, rollback_table_size);

    
    for (uint64_t i = 0; i < rollback_table_size; i++) {
      if (rollforward_table[i].from == ra_dst) {
	ra_src = rollforward_table[i].to;
	break;
      }
    }

    if (ra_src == NULL) {
      printf("found no entry in rollforward table!\n");
    }
    //    dump_table();
    exit(1);
  }
}

#define unlikely(x)    __builtin_expect(!!(x), 0)

#define D 64

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
    if (promoted) {
      return;
    }
  }
 exit:
  *dst = r;
}

auto rf_well_formed_check() {
  if (rollforward_table_size == 0) {
    return;
  }
  
  uint64_t rff1 = (uint64_t)rollforward_table[0].from;
  for (uint64_t i = 1; i < rollforward_table_size; i++) {
    uint64_t rff2 = (uint64_t)rollforward_table[i].from;
    // check increasing order of 'from' keys in rollforwad table
    if (rff2 < rff1) {
      printf("bogus ordering in rollforward table rff2=%lx rff1=%lx\n",rff2,rff1);
      //      exit(1);
    }
    // check that rollback table is an inverse mapping of the
    // rollforward table
    uint64_t rft2 = (uint64_t)rollforward_table[i].to;
    uint64_t rbf2 = (uint64_t)rollback_table[i].from;
    uint64_t rbt2 = (uint64_t)rollback_table[i].to;
    if (rft2 != rbf2) {
      printf("bogus mapping rft2=%lx rbf2=%lx\n",rft2,rbf2);
      exit(1);
    }
    if (rff2 != rbt2) {
      printf("bogus mapping!\n");
      exit(1);
    }
    rff1 = rff2;
  }
  //  printf("passed rf table checks\n");
}

int sorter(const void* v1,const void* v2) {
  return *((uint64_t*)v1)-*((uint64_t*)v2);
}

int main() {
  uint64_t n = taskparts::cmdline::parse_or_default_long("n", 1000 * 1000 * 1000);
  double result = 0.0;
  double* a;

  qsort(rollforward_table, rollforward_table_size, 16, sorter);
  qsort(rollback_table, rollforward_table_size, 16, sorter);
  rf_well_formed_check();
  
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    sum_array_heartbeat(a, 0, n, 0.0, &result);
  }, [&] (auto sched) {
    a = new double[n];
    taskparts::parallel_for(0, n, [&] (size_t i) {
      a[i] = 1.0;
    }, taskparts::dflt_parallel_for_cost_fn, sched);
  }, [&] (auto sched) {
    delete [] a;
  });
  
  printf("result=%f\n",result);
  return 0;
}
