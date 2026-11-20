// What a vertex costs, when its body is empty.
//
//   vertices forks N    a parallel loop of N iterations that do nothing,
//                       made of binary forks: the baseline of the next two
//   vertices finish N   the same loop, whose iteration i makes a vertex with
//                       an empty body, adds an edge from it to one vertex f
//                       that all iterations share, and releases it; the
//                       program then waits for f. This is async-finish with
//                       N asyncs in one finish, so every iteration adds to
//                       the in-counter and to nothing else that is shared.
//   vertices each N     N times in a row: make a vertex with an empty body,
//                       release it, and wait for it
//
// Prints the time of the region and the time per iteration.

#include "microtaskparts.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using namespace microtaskparts;

template <typename Body> void loop(size_t lo, size_t hi, const Body &body) {
  if (hi - lo == 1) {
    body(lo);
    return;
  }
  size_t mid = lo + (hi - lo) / 2;
  fork2join([&] { loop(lo, mid, body); }, [&] { loop(mid, hi, body); });
}

int main(int argc, char **argv) {
  const char *mode = (argc > 1) ? argv[1] : "finish";
  size_t n = (argc > 2) ? (size_t)std::atol(argv[2]) : 1000000;
  if (n < 1) {
    std::fprintf(stderr, "vertices: N must be at least 1\n");
    return 2;
  }
  std::vector<vertex *> made(n, nullptr);
  size_t nb_ran = 0;
  snapshot before = take_snapshot(); // starts the worker threads
  if (std::strcmp(mode, "forks") == 0) {
    launch([&] { loop(0, n, [&](size_t i) { made[i] = nullptr; }); });
  } else if (std::strcmp(mode, "finish") == 0) {
    launch([&] {
      vertex *f = new_vertex([] {});
      loop(0, n, [&](size_t i) {
        vertex *a = new_vertex([] {});
        made[i] = a;
        new_edge(a, f);
        release(a);
      });
      release(f);
      wait(f);
      delete f;
    });
  } else if (std::strcmp(mode, "each") == 0) {
    launch([&] {
      for (size_t i = 0; i < n; i++) {
        vertex *v = new_vertex([&] { nb_ran++; });
        release(v);
        wait(v);
        delete v;
      }
    });
  } else {
    std::fprintf(stderr, "vertices: unknown mode %s\n", mode);
    return 2;
  }
  stats st = stats_between(before, take_snapshot());
  double exectime = st.exectime;
  size_t nb_made = 0;
  for (vertex *v : made) {
    nb_made += (v != nullptr) ? 1 : 0;
    delete v;
  }
  bool ok = (std::strcmp(mode, "finish") == 0)  ? (nb_made == n)
            : (std::strcmp(mode, "each") == 0) ? (nb_ran == n)
                                               : (nb_made == 0);
  std::printf("vertices %s n=%zu\n", mode, n);
  std::printf("workers   %zu\n", get_nb_workers());
  std::printf("exectime  %.3f s\n", exectime);
  std::printf("each      %.1f ns\n", 1e9 * exectime / (double)n);
  print_stats(stdout, st);
  std::printf("check     %s\n", ok ? "ok" : "WRONG");
  return ok ? 0 : 1;
}
