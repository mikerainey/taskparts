// A program whose parallelism comes in bursts: it alternates a phase with no
// parallelism and a phase with plenty, each doing the same work, fib(n).

#include "microtaskparts.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>

#include <sys/resource.h>

using namespace microtaskparts;

__attribute__((noinline)) auto fib_serial(long n) -> long {
  return (n < 2) ? n : fib_serial(n - 1) + fib_serial(n - 2);
}

auto fib(long n) -> long {
  if (n < 20) {
    return fib_serial(n);
  }
  long a, b;
  fork2join([&] { a = fib(n - 1); }, [&] { b = fib(n - 2); });
  return a + b;
}

auto cpu_time() -> double {
  struct rusage ru;
  getrusage(RUSAGE_SELF, &ru);
  return (double)(ru.ru_utime.tv_sec + ru.ru_stime.tv_sec) +
         1e-6 * (double)(ru.ru_utime.tv_usec + ru.ru_stime.tv_usec);
}

int main(int argc, char **argv) {
  long n = (argc > 1) ? std::atol(argv[1]) : 30;
  int nb_rounds = (argc > 2) ? std::atoi(argv[2]) : 20;
  long expected = fib_serial(n);
  long nb_wrong = 0;
  auto start = std::chrono::steady_clock::now();
  double cpu_start = cpu_time();
  // One launch for the whole run, so that the workers live through the
  // serial phases.
  volatile long input = n; // read at every use, so that no phase is optimized away
  launch([&] {
#if defined(MTP_STATS)
    snapshot before = take_snapshot();
#endif
    for (int round = 0; round < nb_rounds; round++) {
      nb_wrong += (fib_serial(input) != expected);
      nb_wrong += (fib(input) != expected);
    }
#if defined(MTP_STATS)
    // The totals of the rounds, from one moment with every other worker
    // asleep to the next.
    print_stats(stdout, stats_between(before, take_snapshot()));
#endif
  });
  std::chrono::duration<double> elapsed =
      std::chrono::steady_clock::now() - start;
  std::printf("bursty(%ld, %d): %s\n", n, nb_rounds,
              (nb_wrong == 0) ? "ok" : "WRONG");
  std::printf("workers   %zu\n", get_nb_workers());
  std::printf("exectime  %.3f s\n", elapsed.count());
  std::printf("cputime   %.3f s\n", cpu_time() - cpu_start);
  return (nb_wrong == 0) ? 0 : 1;
}
