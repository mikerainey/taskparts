#pragma once

#include <deque>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <memory>

#include "scheduler.hpp"
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

// logging events defined in mcsl_scheduler.hpp

using program_point_type = struct program_point_struct {
  
  int line_nb;
  
  const char* source_fname;

  void* ptr;
      
};

static constexpr
program_point_type dflt_ppt = { .line_nb = -1, .source_fname = nullptr, .ptr = nullptr };

uint64_t basetime;

class event_type {
public:
  
  uint64_t cyclecount;
  
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
    auto s = cycles::seconds_of(cycles::diff(basetime, cyclecount));
    fprintf(f, "%lu.%lu\t%ld\t%s\t", s.whole_part, s.fractional_part, worker_id, name_of(tag).c_str());
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
    auto ns = cycles::nanoseconds_of(cycles::diff(basetime, cyclecount));
    switch (tag) {
      case enter_wait:
      case exit_wait: {
	print_hdr();
	print_json_string("name", "idle");
	print_json_string("cat", "SCHED");
	print_json_string("ph", (tag == enter_wait) ? "B" : "E");
	print_json_number("pid", 0);
	print_json_number("tid", worker_id);
	print_json_number("ts", ns, true);
	print_ftr();
        break;
      }
      case enter_launch:
      case exit_launch: {
	print_hdr();
	print_json_string("name", (tag == enter_launch) ? "launch_begin" : "launch_end");
	print_json_string("cat", "SCHED");
	print_json_string("ph", "i");
	print_json_number("pid", 0);
	print_json_number("tid", worker_id);
	print_json_number("ts", ns);
	print_json_string("s", "g", true);
	print_ftr();
        break;
      }
      case enter_sleep:
      case exit_sleep: {
	print_hdr();
	print_json_string("name", "sleep");
	print_json_string("cat", "SCHED");
	print_json_string("ph", (tag == enter_sleep) ? "B" : "E");
	print_json_number("pid", 0);
	print_json_number("tid", worker_id);
	print_json_number("ts", ns, true);
	print_ftr();
        break;
      }
      default: {
        break;
      }
    }
  }

};

/*---------------------------------------------------------------------*/
/* Log buffer */
  
using buffer_type = std::deque<event_type>;
  
static constexpr
int max_nb_ppts = 50000;

template <bool enabled>
class logging_base {
public:
  
  static
  bool real_time;
  
  static
  perworker::array<buffer_type> buffers;
  
  static
  bool tracking_kind[nb_kinds];
  
  static
  program_point_type ppts[max_nb_ppts];

  static
  int nb_ppts;
    
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
    e.cyclecount = cycles::now();
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

  static
  void initialize(bool _real_time=false, bool log_phases=true, bool log_fibers=false) {
    if (! enabled) {
      return;
    }
    real_time = _real_time;
    tracking_kind[phases] = log_phases;
    tracking_kind[fibers] = log_fibers;
    reset();
  }

  static
  auto reset() {
    for (auto id = 0; id != perworker::nb_workers(); id++) {
      buffers[id].clear();
    }
    basetime = cycles::now();
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
    for (auto i = 0; i < nb_ppts; i++) {
      event_type e(program_point);
      e.extra.ppt = ppts[i];
      push(e);
    }
    buffer_type b;
    for (auto id = 0; id != perworker::nb_workers(); id++) {
      buffer_type& b_id = buffers[id];
      for (auto e : b_id) {
        b.push_back(e);
      }
    }
    std::stable_sort(b.begin(), b.end(), [] (const event_type& e1, const event_type& e2) {
      return e1.cyclecount < e2.cyclecount;
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

template <bool enabled>
int logging_base<enabled>::nb_ppts = 0;

template <bool enabled>
program_point_type logging_base<enabled>::ppts[max_nb_ppts];

  /*
static inline
void log_program_point(int line_nb,
                        const char* source_fname,
                        void* ptr) {
  if ((line_nb == -1) || (log_buffer::nb_ppts >= max_nb_ppts)) {
    return;
  }
  program_point_type ppt;
  ppt.line_nb = line_nb;
  ppt.source_fname = source_fname;
  ppt.ptr = ptr;
  log_buffer::ppts[log_buffer::nb_ppts++] = ppt;
}
  */
  
} // end namespace
