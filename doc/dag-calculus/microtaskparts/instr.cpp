// A test of the worker threads, which outlive a computation, and of the
// instrumentation. It runs many computations in a row, and, in a build that
// records them, checks that the times of a region add up and that the counts
// are the ones the program made.
//
//   instr [ROUNDS]     prints "ok", or what is wrong
//   instr orphan       releases a vertex that nothing waits for: a build with
//                      MTP_CHECK_ORPHANS must stop

#include "microtaskparts.hpp"

#include <atomic>
#include <cmath>
#include <cstdio>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>

using namespace microtaskparts;

static int nb_wrong = 0;

#define EXPECT(cond, ...)                                                     \
  do {                                                                        \
    if (!(cond)) {                                                            \
      nb_wrong++;                                                             \
      std::printf("WRONG: " __VA_ARGS__);                                     \
      std::printf("\n");                                                      \
    }                                                                         \
  } while (0)

static long fib(long n) {
  if (n < 2) {
    return n;
  }
  long a, b;
  fork2join([&] { a = fib(n - 1); }, [&] { b = fib(n - 2); });
  return a + b;
}

// The three times add up to the time of the region on every worker.
[[maybe_unused]] static void check_times(const stats &s, int round,
                                         const char *what) {
  double total = s.work + s.sched + s.idle;
  double expected = (double)s.nb_workers * s.exectime;
  EXPECT(std::fabs(total - expected) <= 0.02 * expected + 1e-4,
         "round %d, %s: work + sched + idle = %.6f, P * T = %.6f", round, what,
         total, expected);
  EXPECT(s.work > 0.0, "round %d, %s: no work recorded", round, what);
}

int main(int argc, char **argv) {
  if ((argc > 1) && (std::strcmp(argv[1], "orphan") == 0)) {
    launch([] {
      release(new_vertex([] {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
      }));
    });
    std::printf("the computation ended\n");
    return 0;
  }
  int nb_rounds = (argc > 1) ? std::atoi(argv[1]) : 3;
  for (int round = 0; round < nb_rounds; round++) {
    // 1. Edges and waits: n vertices, and one last vertex that depends on
    // all of them and is waited for twice.
    const int n = 200;
    std::atomic<int> nb_done{0};
    bool last_ran = false;
    snapshot a = take_snapshot();
    launch([&] {
      vertex *last = new_vertex([&] { last_ran = (nb_done.load() == n); });
      std::vector<vertex *> vs;
      for (int i = 0; i < n; i++) {
        vs.push_back(new_vertex([&] {
          volatile long x = fib(10);
          (void)x;
          nb_done.fetch_add(1);
        }));
        new_edge(vs.back(), last);
      }
      for (vertex *v : vs) {
        release(v);
      }
      release(last);
      wait(last);
      wait(last); // has finished: does not suspend
      for (vertex *v : vs) {
        delete v;
      }
      delete last;
    });
    EXPECT(last_ran, "round %d: the last vertex ran early or not at all",
           round);
    snapshot b = take_snapshot();
#if defined(MTP_STATS)
    {
      stats s = stats_between(a, b);
      check_times(s, round, "vertices");
      EXPECT(s.nb_resumes <= 1, "round %d: %llu resumes", round,
             (unsigned long long)s.nb_resumes);
      EXPECT(s.nb_stolen_vertices + s.nb_stolen_stacks <= s.nb_steals,
             "round %d: more stolen vertices and stacks than steals", round);
#if defined(MTP_COUNT_DAG)
      // The root and the sink of launch are vertices too.
      EXPECT(s.nb_vertices == (uint64_t)n + 3,
             "round %d: %llu vertices, not %d", round,
             (unsigned long long)s.nb_vertices, n + 3);
      EXPECT(s.nb_edges + s.nb_edges_late == (uint64_t)n + 3,
             "round %d: %llu edges and %llu late", round,
             (unsigned long long)s.nb_edges,
             (unsigned long long)s.nb_edges_late);
      EXPECT(s.nb_edges >= (uint64_t)n, "round %d: %llu edges", round,
             (unsigned long long)s.nb_edges);
      EXPECT(s.nb_waits == 2, "round %d: %llu waits", round,
             (unsigned long long)s.nb_waits);
      EXPECT(s.nb_suspends == s.nb_resumes,
             "round %d: %llu suspends, %llu resumes", round,
             (unsigned long long)s.nb_suspends,
             (unsigned long long)s.nb_resumes);
      EXPECT(s.nb_migrations <= s.nb_suspends, "round %d: %llu migrations",
             round, (unsigned long long)s.nb_migrations);
#endif
    }
#endif

    // 2. Fork join alone: a region that holds two computations and the
    // sequential code between them.
    a = take_snapshot();
    long r = fib(22);
    volatile long seq = 0;
    for (long i = 0; i < 2000000; i++) {
      seq = seq + i;
    }
    long r2 = fib(22);
    b = take_snapshot();
    EXPECT((r == 17711) && (r2 == 17711), "round %d: fib(22) = %ld, %ld",
           round, r, r2);
#if defined(MTP_STATS)
    {
      stats s = stats_between(a, b);
      check_times(s, round, "fork join");
      EXPECT(s.nb_resumes == 0, "round %d: %llu resumes in fork join", round,
             (unsigned long long)s.nb_resumes);
#if defined(MTP_COUNT_FORKS)
      EXPECT(s.nb_forks == 2 * 28656, "round %d: %llu forks", round,
             (unsigned long long)s.nb_forks);
#endif
#if defined(MTP_COUNT_DAG)
      EXPECT(s.nb_vertices == 4, "round %d: %llu vertices in fork join", round,
             (unsigned long long)s.nb_vertices);
      EXPECT(s.nb_fork_foreign == 0, "round %d: %llu foreign pops", round,
             (unsigned long long)s.nb_fork_foreign);
      EXPECT(s.nb_fork_switches <= 2 * s.nb_steals + 2,
             "round %d: %llu forks switched to the loop, with %llu steals",
             round, (unsigned long long)s.nb_fork_switches,
             (unsigned long long)s.nb_steals);
#endif
#if defined(MTP_LOG)
      if (const char *path = std::getenv("MTP_TRACE")) {
        write_trace(path, a, b);
      }
#endif
    }
#endif

    // 3. Many small computations in a row: each must run its root, and end
    // only once its root has. The frame of launch, which holds the root and
    // the sink, is reused from one to the next.
    for (int i = 0; i < 20000; i++) {
      int ran = 0;
      launch([&] { ran = i + 1; });
      if (ran != i + 1) {
        EXPECT(false, "round %d: computation %d ended before its root ran",
               round, i);
        break;
      }
    }
    {
      long sum = 0;
      for (int i = 0; i < 2000; i++) {
        long x = 0;
        long y = 0;
        fork2join([&] { x = 1; }, [&] { y = 2; });
        sum += x + y;
      }
      EXPECT(sum == 6000, "round %d: 2000 small forks summed to %ld", round,
             sum);
    }
  }
  if (nb_wrong == 0) {
    std::printf("ok\n");
  }
  return (nb_wrong == 0) ? 0 : 1;
}
