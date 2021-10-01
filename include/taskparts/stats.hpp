#pragma once

#include <cstdio>
#include <sys/time.h>
#include <sys/resource.h>

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

  using rusage_type = struct rusage;

  using summary_type = struct summary_struct {
    uint64_t counters[Configuration::nb_counters];
    double exectime;
    timestamp_type total_work_time;
    timestamp_type total_idle_time;
    double total_time;
    double utilization;
    rusage_type rusage_before;
    rusage_type rusage_after;
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
  steadyclock::time_point_type enter_launch_time;

  static
  rusage_type ru_launch_time;
  
  using private_timers = struct private_timers_struct {
    timestamp_type start_work;
    timestamp_type total_work_time;
    timestamp_type start_idle;
    timestamp_type total_idle_time;
  };

  static
  perworker::array<private_timers> all_timers;

  static
  std::vector<summary_type> summaries;

public:

  static inline
  auto increment(counter_id_type id) {
    if (! Configuration::collect_all_stats) {
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
    if (! Configuration::collect_all_stats) {
      return;
    }
    all_timers.mine().start_idle = now();
  }
  
  static
  auto on_exit_acquire() {
    if (! Configuration::collect_all_stats) {
      return;
    }
    auto& t = all_timers.mine();
    t.total_idle_time += since(t.start_idle);
  }

  static
  auto on_enter_work() {
    if (! Configuration::collect_all_stats) {
      return;
    }
    all_timers.mine().start_work = now();
  }
  
  static
  auto on_exit_work() {
    if (! Configuration::collect_all_stats) {
      return;
    }
    auto& t = all_timers.mine();
    t.total_work_time += since(t.start_work);
  }

  static
  auto start_collecting() {
    getrusage(RUSAGE_SELF, &ru_launch_time);
    enter_launch_time = steadyclock::now();
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
    summary.exectime = steadyclock::since(enter_launch_time);
    getrusage(RUSAGE_SELF, &(summary.rusage_after));
    summary.rusage_before = ru_launch_time;
    if (! Configuration::collect_all_stats) {
      return summary;
    }
    for (int counter_id = 0; counter_id < Configuration::nb_counters; counter_id++) {
      uint64_t counter_value = 0;
      for (size_t i = 0; i < nb_workers; ++i) {
        counter_value += all_counters[i].counters[counter_id];
      }
      summary.counters[counter_id] = counter_value;
    }
    double cumulated_time = summary.exectime * nb_workers;
    timestamp_type total_work_time = 0;
    timestamp_type total_idle_time = 0;
    auto my_id = perworker::my_id();
    for (size_t i = 0; i < nb_workers; ++i) {
      auto& t = all_timers[i];
      total_work_time += t.total_work_time;
      total_idle_time += t.total_idle_time;
    }
    double relative_idle = cycles::seconds_of_nanoseconds(total_idle_time) / cumulated_time;
    double utilization = 1.0 - relative_idle;
    summary.total_work_time = total_work_time;
    summary.total_idle_time = total_idle_time;
    summary.total_time = cumulated_time;
    summary.utilization = utilization;
    return summary;
  }

  static
  auto capture_summary() {
    summaries.push_back(report());
  }

  static
  auto output_summary(summary_type summary, FILE* f) {
    auto output_before = [&] {
      fprintf(f, "{");
    };
    auto output_after = [&] (bool not_last) {
      if (not_last) {
	fprintf(f, ",\n");
      } else {
	fprintf(f, "}");
      }
    };
    auto output_uint64_value = [&] (const char* n, uint64_t v, bool not_last = true) {
      fprintf(f, "\"%s\": %lu", n, v);
      output_after(not_last);
    };
    auto output_double_value = [&] (const char* n, double v, bool not_last = true) {
      fprintf(f, "\"%s\": %.3f", n, v);
      output_after(not_last);
    };
    auto output_cycles_in_seconds = [&] (const char* n, uint64_t cs, bool not_last = true) {
      double s = cycles::seconds_of_cycles(cs);
      output_double_value(n, s, not_last);
    };
    auto output_rusage_tv = [&] (const char* n, struct timeval before, struct timeval after,
				 bool not_last = true) {
      auto double_of_tv = [] (struct timeval tv) {
	return ((double) tv.tv_sec) + ((double) tv.tv_usec)/1000000.;
      };
      output_double_value(n, double_of_tv(after) - double_of_tv(before), not_last);
    };
    output_before();
    output_double_value("exectime", summary.exectime);
    output_rusage_tv("usertime", summary.rusage_before.ru_utime, summary.rusage_after.ru_utime);
    output_rusage_tv("systime", summary.rusage_before.ru_stime, summary.rusage_after.ru_stime);
    output_uint64_value("nvcsw", summary.rusage_after.ru_nvcsw - summary.rusage_before.ru_nvcsw);
    output_uint64_value("nivcsw", summary.rusage_after.ru_nivcsw - summary.rusage_before.ru_nivcsw);
    output_uint64_value("maxrss", summary.rusage_after.ru_maxrss);
    output_uint64_value("nsignals", summary.rusage_after.ru_nsignals - summary.rusage_before.ru_nsignals,
			Configuration::collect_all_stats);
    if (! Configuration::collect_all_stats) {
      return;
    }
    for (int i = 0; i < Configuration::nb_counters; i++) {
      output_uint64_value(Configuration::name_of_counter((counter_id_type)i), summary.counters[i]);
    }
    output_cycles_in_seconds("total_work_time", summary.total_work_time);
    output_cycles_in_seconds("total_idle_time", summary.total_idle_time);
    output_double_value("total_time", summary.total_time);
    output_double_value("utilization", summary.utilization, false);
  }

  static
  auto output_summaries() {
    std::string outfile = "";
    if (const auto env_p = std::getenv("TASKPARTS_STATS_OUTFILE")) {
      outfile = std::string(env_p);
    }
    FILE* f = (outfile == "") ? stdout : fopen(outfile.c_str(), "w");
    fprintf(f, "[");
    size_t i = 0;
    for (auto s : summaries) {
      output_summary(s, f);
      if (i + 1 != summaries.size()) {
	fprintf(f, ",");
      }
      i++;
    }
    fprintf(f, "]\n");
    if (f != stdout) {
      fclose(f);
    }
    summaries.clear();
  }
  
};

template <typename Configuration>
std::vector<typename stats_base<Configuration>::summary_type> stats_base<Configuration>::summaries;

template <typename Configuration>
perworker::array<typename stats_base<Configuration>::private_counters> stats_base<Configuration>::all_counters;

template <typename Configuration>
typename stats_base<Configuration>::rusage_type stats_base<Configuration>::ru_launch_time;

template <typename Configuration>
steadyclock::time_point_type stats_base<Configuration>::enter_launch_time;

template <typename Configuration>
perworker::array<typename stats_base<Configuration>::private_timers> stats_base<Configuration>::all_timers;

} // end namespace
