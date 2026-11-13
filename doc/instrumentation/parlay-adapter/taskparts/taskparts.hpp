#pragma once

// Presents the micro taskparts of ../../microtaskparts to ParlayLib under the
// name of TaskPaRTS.
//
// ParlayLib's scheduler plugin for TaskPaRTS (PARLAY_TASKPARTS) includes this
// header, and needs three functions in the namespace taskparts. That is all
// there is to it: the workers of this micro taskparts outlive an outermost
// call to fork2join, and their number is known before the first one, so a
// ParlayLib program, which makes many outermost calls and sizes tables by
// the number of workers, runs as it is.

#include "microtaskparts.hpp"

namespace taskparts {
using microtaskparts::fork2join;
using microtaskparts::get_my_id;
using microtaskparts::get_nb_workers;
} // namespace taskparts
