#pragma once

namespace taskparts {

using ping_thread_status_type = enum ping_thread_status_enum {
  ping_thread_status_active,
  ping_thread_status_exit_launch,
  ping_thread_status_exited
};

static constexpr
uint64_t dflt_kappa_usec = 100;
  
uint64_t kappa_usec = dflt_kappa_usec, kappa_cycles;

static inline
void assign_kappa(uint64_t cpu_freq_khz, uint64_t _kappa_usec=dflt_kappa_usec) {
  kappa_usec = _kappa_usec;
  uint64_t cycles_per_usec = cpu_freq_khz / 1000l;
  kappa_cycles = cycles_per_usec * kappa_usec;
}

} // end namespace

#if defined(TASKPARTS_POSIX)
#include "posix/tpalrts.hpp"
#elif defined (TASKPARTS_NAUTILUS)
#include "nautilus/tpalrts.hpp"
#else
#error need to declare platform (e.g., TASKPARTS_POSIX)
#endif

