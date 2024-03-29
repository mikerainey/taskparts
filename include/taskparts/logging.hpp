#pragma once

#include <deque>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <memory>

#include "posix/diagnostics.hpp"
#include "scheduler.hpp" // logging events are defined here
#include "machine.hpp"
#include "diagnostics.hpp"

// Important: all timestamps are raw cycle counts. We can do so because x64
// multicore systems provide timestamps via rdtsc and those timestamps are
// portable across cores.

// The json output is to be visualized by Google Chrome's tracer utility,
// which is accessed by entering chrome://tracing in the address box. The
// json format is described here:
// https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/preview

namespace taskparts {

using program_point_type = struct program_point_struct {
  
  int line_nb;
  
  const char* source_fname;

  void* ptr;
      
};

class event_type {
public:

  static
  uint64_t base_time;
  
  uint64_t cycle_count;
  
  event_tag_type tag;
  
  size_t worker_id;
  
  event_type() { }
  
  event_type(event_tag_type tag)
  : tag(tag) { }
  
  union extra_union {
    program_point_type ppt;
    size_t child_id;
    struct enter_sleep_struct {
      size_t parent_id;
      size_t prio_child;
      size_t prio_parent;
    } enter_sleep;
    struct failed_to_sleep_struct {
      size_t parent_id;
      size_t busy_child;
      size_t prio_child;
      size_t prio_parent;
    } failed_to_sleep;
  } extra;
            
  void print_text(FILE* f) {
    auto ns = cycles::nanoseconds_of(cycles::diff(base_time, cycle_count)) / 1000;
    fprintf(f, "%.9f\t%ld\t%s\t", ((double)ns) / 1.0e6, worker_id, name_of(tag).c_str());
    switch (tag) {
      case program_point: {
        fprintf(f, "%s \t %d \t %p",
                extra.ppt.source_fname,
                extra.ppt.line_nb,
                extra.ppt.ptr);
        break;
      }
      case wake_child: {
        fprintf(f, "%ld", extra.child_id);
        break;
      }
      case enter_sleep: {
        fprintf(f, "%ld \t %ld \t %ld",
                extra.enter_sleep.parent_id,
                extra.enter_sleep.prio_child,
                extra.enter_sleep.prio_parent);
        break;
      }
      case failed_to_sleep: {
        fprintf(f, "%ld \t Busy[%ld] \t %ld \t %ld",
                extra.failed_to_sleep.parent_id,
                extra.failed_to_sleep.busy_child,
                extra.failed_to_sleep.prio_child,
                extra.failed_to_sleep.prio_parent);
        break;
      }
      default: {
        // nothing to do
      }
    }
    fprintf (f, "\n");
  }

  void print_json(FILE* f, bool last) {
    auto print_json_string = [&] (const char* l, const char* s, bool last_row=false) {
      fprintf(f, "\"%s\": \"%s\"%s", l, s, (last_row ? "" : ","));
    };
    auto print_json_number = [&] (const char* l, size_t n, bool last_row=false) {
      fprintf(f, "\"%s\": %lu%s", l, n, (last_row ? "" : ","));
    };
    auto print_hdr = [&] {
      fprintf(f, "{");
    };
    auto print_ftr = [&] {
      fprintf(f, "}%s", (last ? "\n" : ",\n"));
    };
    // we report in microseconds because it's the unit assumed by Chrome's tracer utility
    auto ns = cycles::nanoseconds_of(cycles::diff(base_time, cycle_count)) / 1000;
    auto print_end = [&] {
      print_json_number("pid", 0);
      print_json_number("tid", worker_id);
      print_json_number("ts", ns, true);
      print_ftr();
    };
    switch (tag) {
      case enter_wait:
      case exit_wait: {
	print_hdr();
	print_json_string("name", "idle");
	print_json_string("cat", "SCHED");
	print_json_string("ph", (tag == enter_wait) ? "B" : "E");
	print_end();
        break;
      }
      case enter_launch:
      case exit_launch: {
	print_hdr();
	print_json_string("name", (tag == enter_launch) ? "launch_begin" : "launch_end");
	print_json_string("cat", "SCHED");
	print_json_string("ph", "i");
	print_json_string("s", "g");
	print_end();
        break;
      }
      case enter_sleep:
      case exit_sleep: {
	print_hdr();
	print_json_string("name", "sleep");
	print_json_string("cat", "SCHED");
	print_json_string("ph", (tag == enter_sleep) ? "B" : "E");
	print_end();
        break;
      }
      case program_point: {
	print_hdr();
	std::string const cstr = extra.ppt.source_fname;
	auto n = cstr + ":" + std::to_string(extra.ppt.line_nb) + " " + std::to_string((size_t)extra.ppt.ptr);
	print_json_string("cat", "PPT");
	print_json_string("name", n.c_str());
	print_json_string("ph", "i");
	print_json_string("s", "t");
	print_end();
        break;
      }
      default: {
        break;
      }
    }
  }

};

uint64_t event_type::base_time;

/*---------------------------------------------------------------------*/
/* Log buffer */
  
using buffer_type = std::deque<event_type>;
  
template <bool enabled>
class logging_base {
public:
  
  static
  bool real_time;
  
  static
  perworker::array<buffer_type> buffers;
  
  static
  bool tracking_kind[nb_kinds];
      
  static inline
  void push(event_type e) {
    if (! enabled) {
      return;
    }
    auto k = kind_of(e.tag);
    assert(k != nb_kinds);
    if (! tracking_kind[k]) {
      return;
    }
    e.cycle_count = cycles::now();
    e.worker_id = perworker::my_id();
    if (real_time) {
      acquire_print_lock();
      e.print_text(stdout);
      release_print_lock();
    }
    buffers.mine().push_back(e);
  }

  static inline
  void log_event(event_tag_type tag) {
    push(event_type(tag));
  }

  static inline
  void log_wake_child(size_t child_id) {
    event_type e(wake_child);
    e.extra.child_id = child_id;
    push(e);
  }

  static inline
  void log_enter_sleep(size_t parent_id, size_t prio_child, size_t prio_parent) {
    event_type e(enter_sleep);
    e.extra.enter_sleep.parent_id = parent_id;
    e.extra.enter_sleep.prio_child = prio_child;
    e.extra.enter_sleep.prio_parent = prio_parent;
    push(e);
  }

  static inline
  void log_failed_to_sleep(size_t parent_id, size_t busybit, size_t prio_child, size_t prio_parent) {
    event_type e(failed_to_sleep);
    e.extra.failed_to_sleep.parent_id = parent_id;
    e.extra.failed_to_sleep.busy_child = busybit;
    e.extra.failed_to_sleep.prio_child = prio_child;
    e.extra.failed_to_sleep.prio_parent = prio_parent;
    push(e);
  }

  static inline
  auto log_program_point(int line_nb, const char* source_fname, void* ptr) -> void {
    program_point_type ppt = { .line_nb = line_nb, .source_fname = source_fname, .ptr = ptr };
    event_type e(program_point);
    e.extra.ppt = ppt;
    push(e);
  }

  static
  void initialize() {
    if (! enabled) {
      return;
    }
    real_time = false;
    if (const auto env_p = std::getenv("TASKPARTS_LOGGING_REALTIME")) {
      real_time = std::stoi(env_p);
    }
    tracking_kind[phases] = true;
    if (const auto env_p = std::getenv("TASKPARTS_LOGGING_PHASES")) {
      tracking_kind[phases] = std::stoi(env_p);
    }
    tracking_kind[fibers] = false;
    if (const auto env_p = std::getenv("TASKPARTS_LOGGING_FIBERS")) {
      tracking_kind[fibers] = std::stoi(env_p);
    }
    tracking_kind[migration] = false;    
    if (const auto env_p = std::getenv("TASKPARTS_LOGGING_MIGRATION")) {
      tracking_kind[migration] = std::stoi(env_p);
    }
    tracking_kind[program] = false;
    if (const auto env_p = std::getenv("TASKPARTS_LOGGING_PROGRAM")) {
      tracking_kind[program] = std::stoi(env_p);
    }
    reset();
  }

  static
  auto reset() {
    for (auto id = 0; id != perworker::nb_workers(); id++) {
      buffers[id].clear();
    }
    event_type::base_time = cycles::now();
    push(event_type(enter_launch));
  }

  static
  void output_json(buffer_type& b, std::string fname) {
    if (fname == "") {
      return;
    }
    FILE* f = fopen(fname.c_str(), "w");
    fprintf(f, "{ \"traceEvents\": [\n");
    size_t i = b.size();
    for (auto e : b) {
      e.print_json(f, (--i == 0));
    }
    fprintf(f, "],\n");
    fprintf(f, "\"displayTimeUnit\": \"ns\"}\n");
    fclose(f);
  }
  
  static
  void output(std::string fname) {
    if (! enabled) {
      return;
    }
    push(event_type(exit_launch));
    buffer_type b;
    for (auto id = 0; id != perworker::nb_workers(); id++) {
      buffer_type& b_id = buffers[id];
      for (auto e : b_id) {
        b.push_back(e);
      }
    }
    std::stable_sort(b.begin(), b.end(), [] (const event_type& e1, const event_type& e2) {
      return e1.cycle_count < e2.cycle_count;
    });
    output_json(b, fname);
  }
  
};

template <bool enabled>
perworker::array<buffer_type> logging_base<enabled>::buffers;

template <bool enabled>
bool logging_base<enabled>::tracking_kind[nb_kinds];

template <bool enabled>
bool logging_base<enabled>::real_time;

} // namespace taskparts

  
