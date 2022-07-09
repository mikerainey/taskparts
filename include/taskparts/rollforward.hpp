#pragma once

#include <sys/signal.h>

#if defined(TASKPARTS_POSIX)
using register_type = greg_t;
#elif defined (TASKPARTS_NAUTILUS)
using register_type = ulong_t*;
#else
#error need to declare platform (e.g., TASKPARTS_POSIX)
#endif

#ifdef TASKPARTS_TPALRTS

/*---------------------------------------------------------------------*/
/* Rollforward table and lookup */

#ifdef TASKPARTS_TPALRTS_HBTIMER_KMOD
extern "C" {
#include <heartbeat.h>
}
#endif

extern
uint64_t rollforward_table_size;
extern
uint64_t rollback_table_size;
extern
struct hb_rollforward rollforward_table[];
extern
struct hb_rollforward rollback_table[];

namespace taskparts {

auto try_to_initiate_rollforward(void** rip) {
  void* ra_src = *rip;
  void* ra_dst = nullptr;
  // Binary search over the rollforward keys
  {
    int64_t i = 0, j = (int64_t)rollforward_table_size - 1;
    int64_t k;
    while (i <= j) {
      k = i + ((j - i) / 2);
      if ((uint64_t)rollforward_table[k].from == (uint64_t)ra_src) {
	ra_dst = rollforward_table[k].to;
	break;
      } else if ((uint64_t)rollforward_table[k].from < (uint64_t)ra_src) {
	i = k + 1;
      } else {
	j = k - 1;
      }
    }
  } 
  if (ra_dst != nullptr) {
    *rip = ra_dst;
  }
}

auto try_to_initiate_rollbackward(void* ra_dst) -> void* {
  void* ra_src = nullptr;
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
  return ra_src;
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

__attribute__((constructor)) // GCC syntax that makes __initialize run before main()
void __initialize(int argc, char **argv) {
  qsort(rollforward_table, rollforward_table_size, 16, sorter);
  qsort(rollback_table, rollforward_table_size, 16, sorter);
  rf_well_formed_check();
}
    
} // end namespace

#define taskparts_tpal_handler \
   __attribute__((preserve_all, noinline))

#define taskparts_tpal_rollbackward \
  { \
  void* ra_dst = __builtin_return_address(0); \
  void* ra_src = taskparts::try_to_initiate_rollbackward(ra_dst); \
  if (ra_src != nullptr) { \
    void* fa = __builtin_frame_address(0); \
    void** rap = (void**)((char*)fa + 8); \
    *rap = ra_src; \
  } else { \
    for (uint64_t i = 0; i < rollback_table_size; i++) { \
      if (rollforward_table[i].from == ra_dst) { \
	ra_src = rollforward_table[i].to; \
	break; \
      } \
    } \
    if (ra_src == nullptr) { \
      printf("found no entry in rollforward table!\n"); \
    } \
    exit(1); \
  } }


#define taskparts_tpal_unlikely(x)    __builtin_expect(!!(x), 0)

#if defined(TASKPARTS_POSIX)
#include "posix/rollforward.hpp"
#elif defined (TASKPARTS_NAUTILUS)
#include "nautilus/rollforward.hpp"
#else
#error need to declare platform (e.g., TASKPARTS_POSIX)
#endif

#endif
