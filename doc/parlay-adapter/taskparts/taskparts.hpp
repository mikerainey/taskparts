#pragma once

// Presents micro taskparts to ParlayLib under the name of TaskPaRTS.
//
// ParlayLib's scheduler plugin for TaskPaRTS (PARLAY_TASKPARTS) includes this
// header, and needs three functions in the namespace taskparts. Here they are
// those of whichever micro taskparts is on the include path.
//
// Micro taskparts starts its workers at each top-level call to fork2join and
// stops them afterwards, and until the first such call its number of workers
// is 1. A ParlayLib program makes many top-level calls, and sizes tables by
// the number of workers. So this header also takes over main: the program's
// own main is renamed, and runs as the root task of one call to launch, with
// the workers running from its first line to its last.

#include "microtaskparts.hpp"

namespace taskparts {
using microtaskparts::fork2join;
using microtaskparts::get_my_id;
using microtaskparts::get_nb_workers;
} // namespace taskparts

int mtp_user_main(int argc, char *argv[]);

// The value mtp_user_main returns is not used. A main may end without a
// return statement, but only under the name main; once renamed, such a
// function returns no particular value. (It must also be compiled with
// -fno-strict-return, which taskpartsConfig.cmake passes, so that the compiler
// does not treat its end as unreachable.) So this main reports success
// whenever the program's main returns at all.
int main(int argc, char *argv[]) {
  microtaskparts::launch([&] { (void)mtp_user_main(argc, argv); });
  return 0;
}

// From here on, a definition of main defines mtp_user_main. It must have the
// form int main(int argc, char *argv[]), and the program a single source file.
#define main mtp_user_main
