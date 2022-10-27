#pragma once

#include <cstddef>
#include <cstdlib>
#include <string>
#include <thread>
#include <assert.h>

#include "diagnostics.hpp"

#ifndef TASKPARTS_MAX_NB_WORKERS_LG
#define TASKPARTS_MAX_NB_WORKERS_LG 8
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
  auto initialize(size_t _nb_workers) {
    if (_nb_workers == 0) {
      taskparts_die("Requested zero worker threads: %lld\n", _nb_workers);
    }    
    if (_nb_workers > default_max_nb_workers) {
      taskparts_die("Requested too many worker threads: %lld, should be maximum %lld\n",
		    _nb_workers, default_max_nb_workers);
    }
    initialize_worker(0);
    nb_workers = (int)_nb_workers;
  }

  static
  auto initialize_worker(size_t id) -> void {
    if (my_id == id) {
      return;
    }
    my_id = (int)id;
  }

  static inline
  auto get_my_id() -> size_t {
    assert(my_id != uninitialized_id);
    return (size_t)my_id;
  }

  static inline
  auto get_nb_workers() -> size_t {
    assert(nb_workers != -1);
    return nb_workers;
  }

};

int id::nb_workers = -1;
  
thread_local
int id::my_id = uninitialized_id;

static
auto nb_workers_requested() -> size_t {
#ifndef TASKPARTS_SERIAL
  if (const auto env_p = std::getenv("TASKPARTS_NUM_WORKERS")) {
    return std::stoi(env_p);
  } else {
    return std::thread::hardware_concurrency();
  }
#else
  return 1;
#endif
}
  
static
__attribute__((constructor))
void init() {
  id::initialize(nb_workers_requested());
}

static
auto my_id() -> size_t {
  return id::get_my_id();
}

static
auto nb_workers() -> size_t {
  return id::get_nb_workers();
}
  
} // end namespace
} // end namespace

#undef TASKPARTS_MAX_NB_WORKERS_LG
