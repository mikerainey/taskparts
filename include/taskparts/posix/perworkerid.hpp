#pragma once

#include <atomic>
#include <cstddef>
#include <assert.h>

#ifndef TASKPARTS_MAX_NB_WORKERS_LG
#define TASKPARTS_MAX_NB_WORKERS_LG 7
#endif

namespace taskparts {
namespace perworker {

/*---------------------------------------------------------------------*/

static constexpr
int default_max_nb_workers_lg = TASKPARTS_MAX_NB_WORKERS_LG;

static constexpr
int default_max_nb_workers = 1 << default_max_nb_workers_lg;

class id {
private:

  static
  int nb_workers;

  static constexpr
  int uninitialized_id = -1;

  static thread_local
  int my_id;

public:
  
  static
  auto initialize(size_t _nb_workers) -> void {
    assert(_nb_workers > 0);
    assert(_nb_workers <= default_max_nb_workers);
    initialize_worker(0);
    nb_workers = _nb_workers;
  }

  static
  auto initialize_worker(size_t id) -> void {
    if (my_id == id) {
      return;
    }
    my_id = id;
  }

  static
  auto get_my_id() -> size_t {
    assert(my_id != uninitialized_id);
    return (size_t)my_id;
  }

  static
  auto get_nb_workers() -> size_t {
    assert(nb_workers != -1);
    return nb_workers;
  }

};

int id::nb_workers = -1;
  
thread_local
int id::my_id = uninitialized_id;

} // end namespace
} // end namespace

#undef TASKPARTS_MAX_NB_WORKERS_LG
