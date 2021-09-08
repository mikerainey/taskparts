#pragma once

#if defined(TASKPARTS_POSIX)
#include "posix/machine.hpp"
#elif defined (TASKPARTS_NAUTILUS)
#include "nautilus/machine.hpp"
#else
#error need to declare platform (e.g., TASKPARTS_POSIX)
#endif

namespace taskparts {

auto pin_calling_worker() {
  // todo
}
  
} // end namespace
