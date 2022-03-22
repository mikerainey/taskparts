#include <taskparts/defaults.hpp>
#include <taskparts/chaselev.hpp>
#include "taskparts/cmdline.hpp"
#include <fib_manualfiber.hpp>

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
  using cl = chase_lev_work_stealing_scheduler<Scheduler, fiber,
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

extern "C"
int func();
__asm__
(
    "_func:\n"
    "    mov w0, #3\n"
    "    ret\n"
);

int main() {
  printf("x=%d\n", func());
  char ctx[arm64_ctx_szb];
  printf("y=%p\n", _taskparts_ctx_save(&ctx[0]));
  taskparts::run();
  return 0;
}
