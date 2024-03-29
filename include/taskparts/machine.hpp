#pragma once

#include <string>
#include <vector>
#include <assert.h>

#include "timing.hpp"
#include "diagnostics.hpp"

namespace taskparts {

/*---------------------------------------------------------------------*/
/* CPU frequency */

namespace {
uint64_t cpu_frequency_khz = 0;
}

static
auto detect_cpu_frequency_khz() -> uint64_t;
  
static inline
auto get_cpu_frequency_khz() -> uint64_t {
  if (cpu_frequency_khz == 0) {
    cpu_frequency_khz = detect_cpu_frequency_khz();
  }
  if (cpu_frequency_khz == 0) {
    auto s =
      "Failed to detect CPU frequency. Correct this issue by assigning" \
      "the environment variable TASKPARTS_CPU_BASE_FREQUENCY_KHZ appropriately.\n";
    taskparts_die(s);    
  }
  return cpu_frequency_khz;
}

/*---------------------------------------------------------------------*/
/* Timing */

namespace cycles {

static inline
auto nanoseconds_of(uint64_t cs) -> uint64_t {
  return nanoseconds_of(get_cpu_frequency_khz(), cs);
}

static inline
auto nanoseconds_since(uint64_t cs) -> uint64_t {
  return nanoseconds_of(since(cs));
}

static inline
auto seconds_of_nanoseconds(uint64_t ns) -> double {
  return (double)ns / 1.0e9;
}

static inline
auto seconds_of_cycles(uint64_t cycles) -> double {
  return seconds_of_nanoseconds(nanoseconds_of(cycles));
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
/* Policies for pinning worker threads to cores or NUMA nodes */

using pinning_policy_type = enum pinning_policy_enum {
  pinning_policy_enabled,
  pinning_policy_disabled
};
  
using resource_packing_type = enum resource_packing_enum {
  resource_packing_sparse,
  resource_packing_dense
};

using resource_binding_type = enum resource_binding_enum {
  resource_binding_all,
  resource_binding_by_core,
  resource_binding_by_numa_node
};

pinning_policy_type pinning_policy = pinning_policy_disabled;

auto pin_calling_worker();
auto initialize_machine();
auto teardown_machine();
  
} // end namespace

#if defined(TASKPARTS_POSIX) || defined(TASKPARTS_DARWIN)
#include "posix/machine.hpp"
namespace taskparts {
auto pin_calling_worker() {
  posix_pin_calling_worker();
}
auto initialize_machine() {
  posix_initialize_machine();
  get_kappa_usec();
}
auto teardown_machine() {
  posix_teardown_machine();
}
} // end namespace
#elif defined (TASKPARTS_NAUTILUS)
#include "nautilus/machine.hpp"
namespace taskparts {
auto pin_calling_worker() {
  nautilus_pin_calling_worker();
}
auto initialize_machine() {
  nautilus_initialize_machine();
  get_kappa_usec();
}
auto teardown_machine() {
  nautilus_teardown_machine();
}
} // end namespace
#else
#error need to declare platform (e.g., TASKPARTS_POSIX)
#endif
