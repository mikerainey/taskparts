#pragma once

#include "timing.hpp"

#if defined(TASKPARTS_POSIX)
#include "posix/machine.hpp"
#elif defined (TASKPARTS_NAUTILUS)
#include "nautilus/machine.hpp"
#else
#error need to declare platform (e.g., TASKPARTS_POSIX)
#endif

namespace taskparts {

/*---------------------------------------------------------------------*/
/* CPU frequency */

namespace {
uint64_t cpu_frequency_khz = 0;
}
  
static inline
auto get_cpu_frequency_khz() -> uint64_t {
  if (cpu_frequency_khz == 0) {
    cpu_frequency_khz = detect_cpu_frequency_khz();
  }
  if (cpu_frequency_khz == 0) {
    taskparts_die("failed to detect CPU frequency\n");
  }
  return cpu_frequency_khz;
}

/*---------------------------------------------------------------------*/
/* Timing */

namespace cycles {

static inline
auto seconds_of(uint64_t cycles) -> seconds_type {
  return seconds_of(get_cpu_frequency_khz(), cycles);
}

static inline
auto nanoseconds_of(uint64_t cycles) -> uint64_t {
  return nanoseconds_of(get_cpu_frequency_khz(), cycles);
}

} // end namespace
    
/*---------------------------------------------------------------------*/
/* Threshold for granularity control */

namespace {
uint64_t kappa_usec = 0;
uint64_t kappa_cycles = 0;
}

auto get_kappa_usec() -> uint64_t {
  if (kappa_usec > 0) {
    return kappa_usec;
  }
  auto assign_kappa = [] (uint64_t cpu_freq_khz, uint64_t _kappa_usec) {
    kappa_usec = _kappa_usec;
    uint64_t cycles_per_usec = cpu_freq_khz / 1000l;
    kappa_cycles = cycles_per_usec * kappa_usec;
  };
  uint64_t dflt_kappa_usec = 100;
  uint64_t k_us = dflt_kappa_usec;
  if (const auto env_p = std::getenv("TASKPARTS_KAPPA_USEC")) {
    k_us = std::stoi(env_p);
  }
  assign_kappa(get_cpu_frequency_khz(), k_us);
  return kappa_usec;
}

auto get_kappa_cycles() -> uint64_t {
  if (kappa_cycles > 0) {
    return kappa_cycles;
  }
  get_kappa_usec();
  return kappa_cycles;
}

/*---------------------------------------------------------------------*/
/* Policies for pinning worker threads to cores */

auto pin_calling_worker() {
  // todo
}
  
} // end namespace
