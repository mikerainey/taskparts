#pragma once

#if defined(TASKPARTS_POSIX)
#include "posix/machine.hpp"
#elif defined (TASKPARTS_NAUTILUS)
#include "nautilus/machine.hpp"
#else
#error need to declare platform (e.g., TASKPARTS_POSIX)
#endif

namespace taskparts {

uint64_t cpu_frequency_khz = 0;

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

auto pin_calling_worker() {
  // todo
}
  
} // end namespace
