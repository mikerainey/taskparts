#pragma once

#include <cstdio>

#include "timing.hpp"
#include "perworker.hpp"
#include "machine.hpp"

namespace taskparts {

template <typename Configuration>
class stats_base {
public:

  using counter_id_type = typename Configuration::counter_id_type;
  
  using configuration_type = Configuration;
  
  using timestamp_type = uint64_t;

  using summary_type = struct summary_struct {
    uint64_t counters[Configuration::nb_counters];
    timestamp_type launch_duration;
    timestamp_type total_work_time;
    timestamp_type total_idle_time;
    timestamp_type total_time;
    double utilization;
  };
  
private:

  static inline
  auto now() -> timestamp_type {
    return cycles::now();
  }

  static inline
  auto since(timestamp_type start) -> timestamp_type {
    return cycles::since(start);
  }

  using private_counters = struct {
    uint64_t counters[Configuration::nb_counters];
  };

  static
  perworker::array<private_counters> all_counters;

  static
  timestamp_type enter_launch_time;
  
  static
  timestamp_type launch_duration;

  using private_timers = struct private_timers_struct {
    timestamp_type start_work;
    timestamp_type total_work_time;
    timestamp_type start_idle;
    timestamp_type total_idle_time;
  };

  static
  perworker::array<private_timers> all_timers;
  
public:

  static inline
  auto increment(counter_id_type id) {
    if (! Configuration::enabled) {
      return;
    }
    all_counters.mine().counters[id]++;
  }

  static inline
  auto on_new_fiber() {
    increment(Configuration::nb_fibers);
  }

  static
  auto on_enter_acquire() {
    if (! Configuration::enabled) {
      return;
    }
    all_timers.mine().start_idle = now();
  }
  
  static
  auto on_exit_acquire() {
    if (! Configuration::enabled) {
      return;
    }
    auto& t = all_timers.mine();
    t.total_idle_time += since(t.start_idle);
  }

  static
  auto on_enter_work() {
    if (! Configuration::enabled) {
      return;
    }
    all_timers.mine().start_work = now();
  }
  
  static
  auto on_exit_work() {
    if (! Configuration::enabled) {
      return;
    }
    auto& t = all_timers.mine();
    t.total_work_time += since(t.start_work);
  }

  static
  auto start_collecting() {
    enter_launch_time = now();
    for (int i = 0; i < all_counters.size(); i++) {
      for (int j = 0; j < Configuration::nb_counters; j++) {
        all_counters[i].counters[j] = 0;
      }
    }
    for (int i = 0; i < all_timers.size(); i++) {
      auto& t = all_timers[i];
      t.start_work = now();
      t.total_work_time = 0;
      t.start_idle = now();
      t.total_idle_time = 0;
    }
  }

  static
  auto report() -> summary_type {
    auto nb_workers = perworker::nb_workers();
    summary_type summary;
    if (! Configuration::enabled) {
      return summary;
    }
    launch_duration = since(enter_launch_time);
    for (int counter_id = 0; counter_id < Configuration::nb_counters; counter_id++) {
      uint64_t counter_value = 0;
      for (size_t i = 0; i < nb_workers; ++i) {
        counter_value += all_counters[i].counters[counter_id];
      }
      summary.counters[counter_id] = counter_value;
    }
    summary.launch_duration = launch_duration;
    timestamp_type cumulated_time = launch_duration * nb_workers;
    timestamp_type total_work_time = 0;
    timestamp_type total_idle_time = 0;
    auto my_id = perworker::my_id();
    for (size_t i = 0; i < nb_workers; ++i) {
      auto& t = all_timers[i];
      total_work_time += t.total_work_time;
      total_idle_time += t.total_idle_time;
    }
    double relative_idle = (double)total_idle_time / (double)cumulated_time;
    double utilization = 1.0 - relative_idle;
    summary.total_work_time = total_work_time;
    summary.total_idle_time = total_idle_time;
    summary.total_time = cumulated_time;
    summary.utilization = utilization;
    return summary;
  }

  static
  auto output_summary(std::string fname="", summary_type summary=report()) {
    FILE* f = (fname == "") ? stdout : fopen(fname.c_str(), "w");
    for (int i = 0; i < Configuration::nb_counters; i++) {
      auto n = Configuration::name_of_counter((counter_id_type)i);
      fprintf(f, "%s %lu\n", n, summary.counters[i]);
    }
    {
      auto s = cycles::seconds_of(summary.launch_duration);
      fprintf(f, "launch_duration %lu.%lu\n", s.whole_part, s.fractional_part);
    }
    {
      auto s = cycles::seconds_of(summary.total_work_time);
      fprintf(f, "total_work_time %lu.%lu\n", s.whole_part, s.fractional_part);
    }
    {
      auto s = cycles::seconds_of(summary.total_idle_time);
      fprintf(f, "total_idle_time %lu.%lu\n", s.whole_part, s.fractional_part);
    }
    {
      auto s = cycles::seconds_of(summary.total_time);
      fprintf(f, "total_time %lu.%lu\n", s.whole_part, s.fractional_part);
    }
    fprintf(f, "utilization %.3f\n", summary.utilization);
    if (fname != "") { // f != stdout
      fclose(f);
    }
  }

};

template <typename Configuration>
perworker::array<typename stats_base<Configuration>::private_counters> stats_base<Configuration>::all_counters;

template <typename Configuration>
typename stats_base<Configuration>::timestamp_type stats_base<Configuration>::enter_launch_time;

template <typename Configuration>
typename stats_base<Configuration>::timestamp_type stats_base<Configuration>::launch_duration;

template <typename Configuration>
perworker::array<typename stats_base<Configuration>::private_timers> stats_base<Configuration>::all_timers;

} // end namespace
