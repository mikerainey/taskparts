#pragma once

#include <chrono>

/*---------------------------------------------------------------------*/
/* Steady clock */

namespace taskparts {
namespace steadyclock {

using time_point_type = std::chrono::time_point<std::chrono::steady_clock>;
  
static inline
double diff(time_point_type start, time_point_type finish) {
  std::chrono::duration<double> elapsed = finish - start;
  return elapsed.count();
}

static inline
time_point_type now() {
  return std::chrono::steady_clock::now();
}

static inline
double since(time_point_type start) {
  return diff(start, now());
}

} // end namespace
} // end namespace
