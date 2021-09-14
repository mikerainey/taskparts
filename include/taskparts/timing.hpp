#pragma once

#include <cstdint>
#include <cstdlib>

#include "diagnostics.hpp"

namespace taskparts {

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
    __asm__("PAUSE");
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

using seconds_type = struct seconds_struct {
  uint64_t whole_part;
  uint64_t fractional_part;
};

static inline
auto seconds_of(uint64_t cpu_frequency_khz, uint64_t cycles) -> seconds_type {
  if (cpu_frequency_khz == 0) {
    taskparts_die("cannot convert from cycles to seconds because cpu frequency is not known\n");
    return {.whole_part = 0, .fractional_part = 0 };
  }
  uint64_t cpms = cycles / cpu_frequency_khz;
  seconds_type t;
  t.whole_part = cpms / 1000l;
  t.fractional_part = cpms - (1000l * t.whole_part);
  return t;
}

static inline
auto nanoseconds_of(uint64_t cpu_frequency_khz, uint64_t cycles) -> uint64_t {
  if (cpu_frequency_khz == 0) {
    taskparts_die("cannot convert from cycles to nanoseconds because cpu frequency is not known\n");
    return 0;
  }
  return 10000l * cycles / cpu_frequency_khz;
}
  
} // end namespace

} // end namespace
