#pragma once

#include <sys/signal.h>

#if defined(TASKPARTS_POSIX)
using register_type = greg_t;
#elif defined (TASKPARTS_NAUTILUS)
using register_type = ulong_t*;
#else
#error need to declare platform (e.g., TASKPARTS_POSIX)
#endif

#ifdef TASKPARTS_TPALRTS

/*---------------------------------------------------------------------*/
/* Rollforward table and lookup */

extern "C" {
#include <rollforward.c>
}

#if defined(TASKPARTS_POSIX)
#include "posix/rollforward.hpp"
#elif defined (TASKPARTS_NAUTILUS)
#include "nautilus/rollforward.hpp"
#else
#error need to declare platform (e.g., TASKPARTS_POSIX)
#endif

#endif
