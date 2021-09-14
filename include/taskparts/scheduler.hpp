#pragma once

#include <string>

#include "timing.hpp"
#include "perworker.hpp"

#if defined(TASKPARTS_POSIX)
#include "posix/minworker.hpp"
#elif defined (TASKPARTS_NAUTILUS)
#include "nautilus/minworker.hpp"
#else
#error need to declare platform (e.g., TASKPARTS_POSIX)
#endif

namespace taskparts {

/*---------------------------------------------------------------------*/
/* Statistics */

class minimal_stats {
public:

  class configuration_type {
  public:
    
    using counter_id_type = enum counter_id_enum {
      nb_fibers,
      nb_steals,
      nb_counters
    };
    
  };

  static
  auto start_collecting() { }

  static
  auto on_enter_launch() { }

  static
  auto on_exit_launch() { }

  static
  auto report(size_t) { }

  static
  auto output_summary() { }

  static inline
  auto on_enter_work() { }

  static inline
  auto on_exit_work() { }

  static inline
  auto on_enter_acquire() { }

  static inline
  auto on_exit_acquire() { }

  static inline
  auto increment(configuration_type::counter_id_type id) { }
  
};

/*---------------------------------------------------------------------*/
/* Logging */

using event_kind_type = enum event_kind_enum {
  phases = 0,
  fibers,
  migration,
  program,
  nb_kinds
};

using event_tag_type = enum event_tag_type_enum {
  enter_launch = 0,   exit_launch,
  enter_algo,         exit_algo,
  enter_wait,         exit_wait,
  worker_communicate, interrupt,
  algo_phase,
  enter_sleep,        exit_sleep,     failed_to_sleep,
  wake_child,         worker_exit,    initiate_teardown,
  program_point,
  nb_events
};

static inline
auto name_of(event_tag_type e) -> std::string {
  switch (e) {
    case enter_launch:      return "enter_launch ";
    case exit_launch:       return "exit_launch ";
    case enter_algo:        return "enter_algo ";
    case exit_algo:         return "exit_algo ";
    case enter_wait:        return "enter_wait ";
    case exit_wait:         return "exit_wait ";
    case enter_sleep:       return "enter_sleep ";
    case failed_to_sleep:   return "failed_to_sleep ";
    case exit_sleep:        return "exit_sleep ";
    case wake_child:        return "wake_child ";
    case worker_exit:       return "worker_exit ";
    case initiate_teardown: return "initiate_teardown";
    case algo_phase:        return "algo_phase ";
    default:                return "unknown_event ";
  }
}

static inline
auto kind_of(event_tag_type e) -> event_kind_type {
  switch (e) {
    case enter_launch:
    case exit_launch:
    case enter_algo:
    case exit_algo:
    case enter_wait:
    case exit_wait:                return phases;
    case enter_sleep:
    case failed_to_sleep:
    case exit_sleep:
    case wake_child:
    case algo_phase:                
    case worker_exit:
    case initiate_teardown:
    case program_point:             return program;
    default:                        return nb_kinds;
  }
}
  
class minimal_logging {
public:

  static
  auto initialize() { }

  static
  auto output(size_t) { }

  static inline
  auto log_event(event_tag_type tag) { }

  static inline
  auto log_enter_sleep(size_t parent_id, size_t prio_child, size_t prio_parent) { }

  static inline
  auto log_wake_child(size_t child_id) { }

};

/*---------------------------------------------------------------------*/
/* Termination detection */

class minimal_termination_detection {
public:

  auto set_active(bool active) -> bool {
    return false;
  }

  auto is_terminated() -> bool{
    return false;
  }
  
};

/*---------------------------------------------------------------------*/
/* Workers (scheduler threads) */

class minimal_worker {
public:

  static
  auto initialize(size_t nb_workers) {
    minimal_worker_initialize(nb_workers);
  }

  static
  auto destroy() {
    minimal_worker_destroy();
  }
  
  template <typename Body>
  static
  auto launch_worker_thread(size_t id, const Body& b) {
    minimal_launch_worker_thread(id, [id, &b] {
      perworker::id::initialize_worker(id);
      pin_calling_worker();
      b(id);
    });
  }

  using worker_exit_barrier = minimal_worker_exit_barrier;
  
  using termination_detection_type = minimal_termination_detection;

};

/*---------------------------------------------------------------------*/
/* Fibers */

using fiber_status_type = enum fiber_status_enum {
  fiber_status_continue,
  fiber_status_pause,
  fiber_status_finish,
  fiber_status_exit_launch
};

template <typename Scheduler>
class minimal_fiber {
public:

  virtual
  fiber_status_type exec() {
    return fiber_status_exit_launch;
  }

  virtual
  void finish() {
    delete this;
  }

  auto is_ready() -> bool {
    return true;
  }
  
};

/*---------------------------------------------------------------------*/
/* Elastic work stealing */

template <typename Stats, typename Logging>
class minimal_elastic {
public:

  static
  auto wake_children() { }

  static
  auto try_to_sleep(size_t) {
    cycles::spin_for(1000);
  }

  static
  auto accept_lifelines() { }

  static
  auto initialize() { }
  
};

/*---------------------------------------------------------------------*/
/* Interrupts */

class minimal_interrupt {
public:

  static
  auto initialize_signal_handler() { }

  static
  auto wait_to_terminate_ping_thread() { }
  
  static
  auto launch_ping_thread(size_t) { }

};
  
} // end namespace
