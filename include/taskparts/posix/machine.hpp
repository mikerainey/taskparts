#pragma once

#include <cstdio>

namespace taskparts {

/*---------------------------------------------------------------------*/
/* Detect CPU frequency */

auto detect_cpu_frequency_khz() -> uint64_t {
  uint64_t cpu_frequency_khz = 0;
  FILE *f;
  f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/base_frequency", "r");
  if (f == nullptr) {
    f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/bios_limit", "r");
  }
  if (f != nullptr) {
    char buf[1024];
    while (fgets(buf, sizeof(buf), f) != 0) {
      sscanf(buf, "%lu", &(cpu_frequency_khz));
    }
    fclose(f);
  }
  return cpu_frequency_khz;
}
  
} // end namespace

