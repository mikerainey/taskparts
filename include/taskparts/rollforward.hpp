#pragma once

#include <sys/signal.h>

#if defined(TASKPARTS_POSIX)
using register_type = greg_t;
#elif defined (TASKPARTS_NAUTILUS)
using register_type = ulong_t*;
#else
#error need to declare platform (e.g., TASKPARTS_POSIX)
#endif

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

void try_to_initiate_rollforward(void** rip) {
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
  if (ra_dst != NULL) {
    *rip = ra_dst;
  }
}
    
} // end namespace

#if defined(TASKPARTS_POSIX)
#include "posix/rollforward.hpp"
#elif defined (TASKPARTS_NAUTILUS)
#include "nautilus/rollforward.hpp"
#else
#error need to declare platform (e.g., TASKPARTS_POSIX)
#endif
