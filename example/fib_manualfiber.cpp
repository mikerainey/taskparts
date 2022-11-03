#include <taskparts/defaults.hpp>
#include <taskparts/workstealing.hpp>
#include "taskparts/cmdline.hpp"
#include <fib_manualfiber.hpp>

/*
clang++ -std=c++17 -g3 -O0 -I. -I../include -DTASKPARTS_DARWIN -DTASKPARTS_ARM64 -DTASKPARTS_STATS fib_manualfiber.cpp
TASKPARTS_CPU_BASE_FREQUENCY_KHZ=3200000 ./a.out
 */

namespace taskparts {

template <
    typename Bench_stats=bench_stats,
    typename Bench_logging=bench_logging,
    template <typename, typename> typename Bench_elastic=bench_elastic,
    typename Bench_worker=bench_worker,
    typename Bench_interrupt=bench_interrupt,
    typename Scheduler=minimal_scheduler<Bench_stats, Bench_logging, Bench_elastic, Bench_worker, Bench_interrupt>>
void run() {
  initialize_machine();
  int64_t dst = 0;
  int64_t n = 45;
  Bench_stats::start_collecting();
  auto f_body = new fib_manualfiber<Scheduler>(n, &dst);
  auto f_term = new terminal_fiber<Scheduler>;
  fiber<Scheduler>::add_edge(f_body, f_term);
  f_body->release();
  f_term->release();
  using cl = work_stealing_scheduler<Scheduler, fiber,
				     Bench_stats, Bench_logging,
				     Bench_elastic, Bench_worker, Bench_interrupt>;
  Bench_stats::on_enter_work();
  cl::launch();
  Bench_stats::on_exit_work();
  Bench_stats::capture_summary();
  Bench_stats::output_summaries();
  teardown_machine();
  printf("dst=%lld P=%lu\n", dst, perworker::nb_workers());
}

} // end namespace taskparts

int main() {
  taskparts::run();
  return 0;
}
