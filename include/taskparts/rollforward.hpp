#pragma once

#include <utility>
#include <algorithm>
#include <vector>

#if defined(TASKPARTS_POSIX)
using register_type = greg_t;
#elif defined (TASKPARTS_NAUTILUS)
using register_type = ulong_t*;
#else
#error need to declare platform (e.g., TASKPARTS_POSIX)
#endif

/*---------------------------------------------------------------------*/
/* Rollforward table and lookup */

namespace taskparts {

using rollforward_edge_type = std::pair<register_type, register_type>;

using rollforward_lookup_table_type = std::vector<rollforward_edge_type>;

auto rollforward_edge_less = [] (const rollforward_edge_type& e1, const rollforward_edge_type& e2) {
  return e1.first < e2.first;
};

template <typename L>
auto mk_rollforward_entry(L src, L dst) -> rollforward_edge_type {
  return std::make_pair((register_type)src, (register_type)dst);
}

// returns the entry dst, if (src, dst) is in the rollforward table t, and src otherwise
static inline
auto lookup_rollforward_entry(const rollforward_lookup_table_type& t, register_type src)
  -> register_type {
  auto dst = src;
  size_t n = t.size();
  if (n == 0) {
    return src;
  }
  static constexpr
  int64_t not_found = -1;
  int64_t k;
  {
    int64_t i = 0, j = (int64_t)n - 1;
    while (i <= j) {
      k = i + ((j - i) / 2);
      if (t[k].first == src) {
	goto exit;
      } else if (t[k].first < src) {
	i = k + 1;
      } else {
	j = k - 1;
      }
    }
    k = not_found;
  }
  exit:
  if (k != not_found) {
    dst = t[k].second;
  }
  return dst;
}
  
// returns the entry src, if (src, dst) is in the rollforward table t, and dst otherwise
auto reverse_lookup_rollforward_entry(const rollforward_lookup_table_type& t, register_type dst)
  -> register_type {
  for (const auto& p : t) {
    if (dst == p.second) {
      return p.first;
    }
  }
  return dst;
}

template <class T>
void try_to_initiate_rollforward(const T& t, register_type* rip) {
  auto ip = *rip;
  auto dst = lookup_rollforward_entry(t, ip);
  if (dst != ip) {
    *rip = dst;
  }
}

rollforward_lookup_table_type rollforward_table;

auto reverse_lookup_rollforward_entry(void* dst) -> void* {
  return (void*)reverse_lookup_rollforward_entry(rollforward_table, (register_type)dst);
}

#ifdef TASKPARTS_TPALRTS_HBTIMER_KMOD
extern "C" {
#include <heartbeat.h>
}
void hbtimer_init_tbl() {
  std::vector<struct hb_rollforward> tbl1;
  for (auto it : rollforward_table) {
    struct hb_rollforward tmp;
    tmp.from = (void*)it.first;
    tmp.to = (void*)it.second;
    tbl1.push_back(tmp);
  }
  hb_set_rollforwards(tbl1.data(), tbl1.size());
}
#endif

auto initialize_rollfoward_table() {
  std::sort(rollforward_table.begin(), rollforward_table.end(), rollforward_edge_less);
}

auto clear_rollforward_table() {
  rollforward_table.clear();
}
    
} // end namespace

#if defined(TASKPARTS_POSIX)
#include "posix/rollforward.hpp"
#elif defined (TASKPARTS_NAUTILUS)
#include "nautilus/rollforward.hpp"
#else
#error need to declare platform (e.g., TASKPARTS_POSIX)
#endif
