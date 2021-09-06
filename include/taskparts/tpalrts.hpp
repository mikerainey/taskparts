#pragma once

namespace taskparts {

using ping_thread_status_type = enum ping_thread_status_enum {
  ping_thread_status_active,
  ping_thread_status_exit_launch,
  ping_thread_status_exited
};

} // end namespace

#if defined(TASKPARTS_POSIX)
#include "posix/tpalrts.hpp"
#elif defined (TASKPARTS_NAUTILUS)
#include "nautilus/tpalrts.hpp"
#else
#error need to declare platform (e.g., TASKPARTS_POSIX)
#endif

