#pragma once

#if defined(TASKPARTS_POSIX) || defined(TASKPARTS_DARWIN)
#include "posix/diagnostics.hpp"
#elif defined (TASKPARTS_NAUTILUS)
#include "nautilus/diagnostics.hpp"
#else
#error need to declare platform (e.g., TASKPARTS_POSIX)
#endif
