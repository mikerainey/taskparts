// fib, the worst case for the cost of a fork. It measures the call of fib
// as a region, between two snapshots. With an instrumented build, it prints
// the stats of the region, and, given a second argument, writes the trace
// there.
//
// Two more builds count forks the wrong way, to show what that costs:
// -DCOUNT_FORKS_ATOMIC counts in one counter shared by all workers, and
// -DCOUNT_FORKS_PACKED in one counter per worker, packed side by side.

#if defined(COUNT_FORKS_ATOMIC)
#include <atomic>
inline std::atomic<unsigned long> shared_nb_forks{0};
#define MTP_ON_FORK(w)                                                         \
  ((void)(w), shared_nb_forks.fetch_add(1, std::memory_order_relaxed))
#elif defined(COUNT_FORKS_PACKED)
inline unsigned long packed_nb_forks[64];
#define MTP_ON_FORK(w) (packed_nb_forks[&(w) - &workers[0]]++)
#endif

#include "microtaskparts.hpp"

#include <cstdio>
#include <cstdlib>

using namespace microtaskparts;

auto fib(long n) -> long {
  if (n < 2) {
    return n;
  }
  long a, b;
  fork2join([&] { a = fib(n - 1); }, [&] { b = fib(n - 2); });
  return a + b;
}

int main(int argc, char **argv) {
  long n = (argc > 1) ? std::atol(argv[1]) : 30;
  snapshot before = take_snapshot();
  long r = fib(n);
  snapshot after = take_snapshot();
  stats s = stats_between(before, after);
  std::printf("fib(%ld) = %ld\n", n, r);
  std::printf("workers   %zu\n", get_nb_workers());
  std::printf("exectime  %.3f s\n", s.exectime);
#if defined(COUNT_FORKS_ATOMIC)
  std::printf("nb_forks  %lu\n", shared_nb_forks.load());
#elif defined(COUNT_FORKS_PACKED)
  unsigned long nb_forks = 0;
  for (size_t i = 0; i < get_nb_workers(); i++) {
    nb_forks += packed_nb_forks[i];
  }
  std::printf("nb_forks  %lu\n", nb_forks);
#endif
#if defined(MTP_STATS)
  print_stats(stdout, s);
#endif
#if defined(MTP_LOG)
  if (argc > 2) {
    write_trace(argv[2], before, after);
  }
#endif
  return 0;
}
