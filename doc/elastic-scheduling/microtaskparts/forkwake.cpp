// A test of the wake at a fork. With every other worker asleep, the root
// task forks once; its first branch waits for the second to start, which
// only a thief can make happen while the first branch runs. So the fork
// must have woken a worker: if it does not, nobody is awake to steal, the
// first branch gives up after a timeout, and the test fails.
//
// Nothing else can wake a worker between the two snapshots of a round: no
// task exists but the one this fork pushes.

#define MTP_STATS // for the counts, and for a snapshot that waits for sleep
#include "microtaskparts.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>

using namespace microtaskparts;

int main(int argc, char **argv) {
  int nb_rounds = (argc > 1) ? std::atoi(argv[1]) : 100;
  double timeout = (argc > 2) ? std::atof(argv[2]) : 2.0; // seconds
  int nb_failed = 0;
  launch([&] {
    if (get_nb_workers() < 2) {
      return;
    }
    for (int round = 0; round < nb_rounds; round++) {
      // Every other worker is asleep from here on.
      snapshot before = take_snapshot();
      std::atomic<bool> started{false};
      bool timed_out = false;
      size_t first = get_my_id();
      size_t second = first;
      fork2join(
          [&] {
            auto start = std::chrono::steady_clock::now();
            while (!started.load()) {
              std::chrono::duration<double> waited =
                  std::chrono::steady_clock::now() - start;
              if (waited.count() > timeout) {
                timed_out = true;
                break;
              }
            }
          },
          [&] {
            second = get_my_id();
            started.store(true);
          });
      stats s = stats_between(before, take_snapshot());
      // One fork ran, so it woke one worker, and one task was stolen.
      bool ok = !timed_out && (second != first) && (s.nb_fork_wakes == 1) &&
                (s.nb_steals == 1);
      if (!ok) {
        nb_failed++;
        std::printf("round %d: %s, second branch on worker %zu, first on %zu; ",
                    round, timed_out ? "timed out" : "no timeout", second,
                    first);
        print_stats(stdout, s);
      }
    }
  });
  if (get_nb_workers() < 2) {
    std::printf("forkwake: needs 2 workers or more\n");
    return 2;
  }
  std::printf("forkwake(%d rounds, %zu workers): %s\n", nb_rounds,
              get_nb_workers(), (nb_failed == 0) ? "ok" : "FAILED");
  return (nb_failed == 0) ? 0 : 1;
}
