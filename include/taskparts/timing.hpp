#pragma once

#include <cstdint>
#include <cstdlib>

#include "diagnostics.hpp"
#if defined(TASKPARTS_POSIX)
#include "posix/steadyclock.hpp"
#elif defined (TASKPARTS_NAUTILUS)
#include "nautilus/steadyclock.hpp"
#else
#error need to declare platform (e.g., TASKPARTS_POSIX)
#endif

namespace taskparts {

static inline
auto busywait_pause() {
#if defined(TASKPARTS_X64)
  __builtin_ia32_pause();
#else
#error need to declare platform (e.g., TASKPARTS_X64)
#endif
}

/*---------------------------------------------------------------------*/
/* Cycle counter */

namespace cycles {

/* x86* specific instructions for getting the current cycle count
 * & waiting
 */
namespace {
  
static inline
auto rdtsc() -> uint64_t {
  unsigned int hi, lo;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return  ((uint64_t) lo) | (((uint64_t) hi) << 32);
}

static inline
auto rdtsc_wait(uint64_t n) {
  const uint64_t start = rdtsc();
  while (rdtsc() < (start + n)) {
    busywait_pause();
  }
}
  
} // end namespace
  
static inline
auto diff(uint64_t start, uint64_t finish) -> uint64_t {
  return finish - start;
}

static inline
auto now() -> uint64_t {
  return rdtsc();
}

static inline
auto since(uint64_t start) -> uint64_t {
  return diff(start, now());
}

static inline
auto spin_for(uint64_t nb_cycles) {
  rdtsc_wait(nb_cycles);
}

static inline
auto nanoseconds_of(uint64_t cpu_frequency_khz, uint64_t cycles) -> uint64_t {
  if (cpu_frequency_khz == 0) {
    taskparts_die("cannot convert from cycles to nanoseconds because cpu frequency is not known\n");
    return 0;
  }
  return 1000000l * cycles / cpu_frequency_khz;
}
  
} // end namespace

} // end namespace
