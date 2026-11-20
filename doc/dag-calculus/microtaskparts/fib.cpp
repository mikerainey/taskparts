#include "microtaskparts.hpp"

#include <chrono>
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
#if defined(MTP_STATS)
  // The first snapshot starts the worker threads, outside the region.
  snapshot before = take_snapshot();
  long r = fib(n);
  stats st = stats_between(before, take_snapshot());
  std::chrono::duration<double> elapsed(st.exectime);
#else
  // As in the native fork join paper's fib, whose times ours are compared
  // with: the region includes the start of the worker threads.
  auto start = std::chrono::steady_clock::now();
  long r = fib(n);
  std::chrono::duration<double> elapsed =
      std::chrono::steady_clock::now() - start;
#endif
  std::printf("fib(%ld) = %ld\n", n, r);
  std::printf("workers   %zu\n", get_nb_workers());
  std::printf("exectime  %.3f s\n", elapsed.count());
#if defined(MTP_STATS)
  print_stats(stdout, st);
#endif
  return 0;
}
