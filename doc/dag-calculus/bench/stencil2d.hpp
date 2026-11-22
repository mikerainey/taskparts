#pragma once

// Stencil computations over a two-dimensional grid, by the trapezoidal
// decomposition of the Pochoir stencil compiler (Tang et al., SPAA 2011),
// which extends that of Frigo and Strumpen (ICS 2005).
//
// The computation fills a region of space and time, a zoid: at each time
// step t it updates the cells of a rectangle whose sides move by at most
// one cell per step. A cell at step t + 1 reads cells at step t that are at
// most one away in each dimension. A zoid is decomposed recursively:
//
//   - by a cut in space, if it is wide enough: a side is cut into two zoids
//     that shrink away from the cut, which do not depend on each other, and
//     the zoid between them, which grows from the cut and depends on both.
//     Pochoir cuts every side that can be cut at once (a hyperspace cut),
//     so a zoid is cut into up to nine;
//   - otherwise by a cut in time, into a lower half and then an upper half;
//   - or not at all, if it is small: then it is run step by step.
//
// Four ways to run a stencil:
//
//   loops    one time step after another, row by row
//   ploops   the same, with the rows of a time step in parallel (fork join)
//   trap     the decomposition, by fork join as in Pochoir: the zoids of a
//            hyperspace cut that have no side between the cuts in
//            parallel, then those with one such side, then the last
//   dag      the same decomposition, by DAG calculus: each zoid of a cut is
//            a vertex, with an edge from each of the zoids that it reads,
//            and no other, so a zoid starts as soon as those are done
//   tbb      trap, with a task group of oneTBB in place of our fork join
//            (in a program built with -DBENCH_TBB)
//
// A program supplies the kernel as a type K with
//
//   static void row(int t, int x, int y0, int y1)
//
// which computes step t + 1 of cells (x, y0) to (x, y1 - 1) from step t.
// It must not be inlined, so that every version runs the same machine code.

#include "harness.hpp"

namespace stencil {

using namespace bench;

// At step t, t0 <= t < t1, the cells (x, y) with
//   x0 + dx0 (t - t0) <= x < x1 + dx1 (t - t0)
// and likewise for y.
struct side {
  int lo, dlo, hi, dhi;
};
struct zoid {
  int t0, t1;
  side x, y;
};

// Where the decomposition stops: a zoid is not cut in time if it has at most
// dt steps, and a side is not cut if it is narrower than 2 * width.
struct cutoffs {
  int dt = 8;
  int width = 64;
};
inline cutoffs cut;

template <typename K> void run_zoid(const zoid &z) {
  side x = z.x, y = z.y;
  for (int t = z.t0; t < z.t1; t++) {
    for (int i = x.lo; i < x.hi; i++) {
      K::row(t, i, y.lo, y.hi);
    }
    x.lo += x.dlo;
    x.hi += x.dhi;
    y.lo += y.dlo;
    y.hi += y.dhi;
  }
}

// Cut a side that lasts dt steps into parts[0] and parts[1], which shrink
// away from the cut, and parts[2], between them. Returns whether it can be:
// the two must not shrink to less than nothing.
inline bool cut_side(const side &s, int dt, side parts[3]) {
  if (s.hi - s.lo < 2 * cut.width) {
    return false;
  }
  int mid = s.lo + (s.hi - s.lo) / 2;
  if ((mid - dt < s.lo + s.dlo * dt) || (mid + dt > s.hi + s.dhi * dt)) {
    return false;
  }
  parts[0] = side{s.lo, s.dlo, mid, -1};
  parts[1] = side{mid, 1, s.hi, s.dhi};
  parts[2] = side{mid, -1, mid, 1};
  return true;
}

// How the zoids of a cut are run.
enum mode { by_fork_join, by_dag, by_tbb };

template <typename K> void walk(const zoid &z, mode m);

// f(k) for 0 <= k < n, in parallel; n is small.
template <typename F> void each(int n, mode m, const F &f) {
#ifdef BENCH_TBB
  if (m == by_tbb) {
    tbb::task_group tasks;
    for (int k = 1; k < n; k++) {
      tasks.run([&f, k] { f(k); });
    }
    if (n > 0) {
      f(0);
    }
    tasks.wait();
    return;
  }
#endif
  parallel_for(0, n, f);
}

template <typename K> void walk(const zoid &z, mode m) {
  int dt = z.t1 - z.t0;
  // The parts of each side: one, or two and the one between them.
  side xs[3] = {z.x}, ys[3] = {z.y};
  int nx = cut_side(z.x, dt, xs) ? 3 : 1;
  int ny = cut_side(z.y, dt, ys) ? 3 : 1;
  if ((nx == 1) && (ny == 1)) {
    if (dt > cut.dt) {
      int half = dt / 2;
      zoid lower = z, upper = z;
      lower.t1 = z.t0 + half;
      upper.t0 = z.t0 + half;
      upper.x.lo += z.x.dlo * half;
      upper.x.hi += z.x.dhi * half;
      upper.y.lo += z.y.dlo * half;
      upper.y.hi += z.y.dhi * half;
      walk<K>(lower, m);
      walk<K>(upper, m);
    } else {
      run_zoid<K>(z);
    }
    return;
  }
  auto part = [&](int a, int b) { return zoid{z.t0, z.t1, xs[a], ys[b]}; };
  // A part between two cuts is numbered 2.
  auto between = [](int n, int a) { return (n == 3) && (a == 2); };
  if (m != by_dag) {
    // In turn: the zoids with no side between cuts, with one, with two.
    for (int level = 0; level <= 2; level++) {
      zoid group[4];
      int n = 0;
      for (int a = 0; a < nx; a++) {
        for (int b = 0; b < ny; b++) {
          if (between(nx, a) + between(ny, b) == level) {
            group[n++] = part(a, b);
          }
        }
      }
      each(n, m, [&](long k) { walk<K>(group[k], m); });
    }
    return;
  }
  // A zoid whose side lies between the cuts reads the two zoids that have
  // the parts beside that one, and the same part of the other side.
  vertex *v[3][3];
  for (int a = 0; a < nx; a++) {
    for (int b = 0; b < ny; b++) {
      zoid p = part(a, b);
      v[a][b] = new_vertex([p] { walk<K>(p, by_dag); });
    }
  }
  for (int a = 0; a < nx; a++) {
    for (int b = 0; b < ny; b++) {
      if (between(nx, a)) {
        new_edge(v[0][b], v[a][b]);
        new_edge(v[1][b], v[a][b]);
      }
      if (between(ny, b)) {
        new_edge(v[a][0], v[a][b]);
        new_edge(v[a][1], v[a][b]);
      }
    }
  }
  for (int a = 0; a < nx; a++) {
    for (int b = 0; b < ny; b++) {
      release(v[a][b]);
    }
  }
  // The last zoid reads, directly or not, all of the others.
  wait(v[nx - 1][ny - 1]);
  for (int a = 0; a < nx; a++) {
    for (int b = 0; b < ny; b++) {
      delete v[a][b];
    }
  }
}

// Run nb_steps steps over the cells (1, 1) to (n, n).
template <typename K> void loops(int n, int nb_steps) {
  for (int t = 0; t < nb_steps; t++) {
    for (int x = 1; x <= n; x++) {
      K::row(t, x, 1, n + 1);
    }
  }
}

template <typename K> void ploops(int n, int nb_steps) {
  // Rows in groups, so that a task is worth its fork.
  int rows = (n + cut.width - 1) / cut.width;
  for (int t = 0; t < nb_steps; t++) {
    parallel_for(0, rows, [&](long g) {
      int hi = std::min(n + 1, (int)(g + 1) * cut.width + 1);
      for (int x = (int)g * cut.width + 1; x < hi; x++) {
        K::row(t, x, 1, n + 1);
      }
    });
  }
}

template <typename K> void trapezoids(int n, int nb_steps, mode m) {
  walk<K>(zoid{0, nb_steps, side{1, 0, n + 1, 0}, side{1, 0, n + 1, 0}}, m);
}

// The whole of a program: K names the stencil, and supplies, besides row,
//   static void initialize(int n)       both grids, n + 2 cells on a side
//   static double checksum(int steps)   of the grid that the last step wrote
//   static bool same(int steps, const void *other)
//   static void *save(int steps)        a copy of that grid
template <typename K> int main_of(const char *name, int argc, char **argv) {
  const char *version = (argc > 1) ? argv[1] : "dag";
  int n = (int)arg(argc, argv, 2, 3000);
  int nb_steps = (int)arg(argc, argv, 3, 500);
  cut.width = (int)arg(argc, argv, 4, cut.width);
  cut.dt = (int)arg(argc, argv, 5, cut.dt);
  bool check = wants_check(argc, argv, 6);
  bool known = is(version, "loops") || is(version, "ploops") ||
               is(version, "trap") || is(version, "dag") ||
               is(version, "tbb");
  if (!known || (n < 1) || (nb_steps < 1) || (cut.width < 1) || (cut.dt < 1)) {
    std::fprintf(stderr, "usage: %s <loops|ploops|trap|dag|tbb> [n] [time steps] "
                         "[width] [dt] [check]\n", name);
    return 2;
  }
  K::initialize(n);
  bool on_tbb = is(version, "tbb");
  bool parallel = !is(version, "loops") && !on_tbb;
  auto body = [&] {
    if (is(version, "loops")) {
      loops<K>(n, nb_steps);
    } else if (is(version, "ploops")) {
      ploops<K>(n, nb_steps);
    } else {
      trapezoids<K>(n, nb_steps, on_tbb ? by_tbb
                                         : (is(version, "dag") ? by_dag
                                                               : by_fork_join));
    }
  };
  region r = on_tbb ? measure_tbb(body) : measure(parallel, body);
  std::printf("%s %s n=%d steps=%d width=%d dt=%d\n", name, version, n,
              nb_steps, cut.width, cut.dt);
  std::printf("checksum  %.9f\n", K::checksum(nb_steps));
  report(r);
  if (check) {
    void *result = K::save(nb_steps);
    K::initialize(n);
    loops<K>(n, nb_steps);
    bool same = K::same(nb_steps, result);
    std::free(result);
    return report_check(same);
  }
  return 0;
}

} // namespace stencil
