#pragma once

#include <cstdlib>

namespace taskparts {
  
template <typename F1, typename F2>
auto fork2join(const F1& f1, const F2& f2) -> void;
extern
auto get_nb_workers() -> size_t;
extern
auto get_my_id() -> size_t;
  
} // end namespace

#include "taskparts_internal.hpp"
