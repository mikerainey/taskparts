// The Cholesky factorization of a dense symmetric positive definite matrix,
// divided into square tiles. The factorization is a graph of three kinds of
// tasks, each on one tile:
//
//   factor(k)        factor the diagonal tile k
//   solve(i, k)      tile (i, k), i > k, against the factored tile k
//   update(i, j, k)  subtract from tile (i, j), i >= j > k, the product of
//                    tiles (i, k) and (j, k)
//
// A task depends on the task that last wrote its tile and on the tasks that
// finished the tiles it reads. Three versions:
//
//   serial  step k after step k - 1, and within a step the factor, then the
//           solves, then the updates;
//   levels  fork join: the same order of steps, with the solves of a step
//           in parallel, then its updates in parallel;
//   dag     DAG calculus: a vertex per task and an edge per dependency, so
//           a task of step k + 1 may run while step k is still at work on
//           other tiles.
//
// The graph is the one that the Cholesky example of Intel's Threading
// Building Blocks builds with its flow graph, and that the KaStORS suite
// (Virouleau et al., IWOMP 2014) expresses with OpenMP's dependent tasks.
// Those call BLAS and LAPACK on each tile; here the three kernels are plain
// loops, so that the program needs no library.
//
// usage: cholesky <serial|levels|dag|tbb> [n] [tile size] [check]

#include "harness.hpp"

#include <cmath>
#include <memory>
#include <vector>

using namespace bench;

static int n, bs, B; // order of the matrix, of a tile, and tiles on a side
static std::vector<double> matrix; // row-major; the lower triangle is used

static double *tile(int i, int j) {
  return &matrix[(size_t)i * bs * n + (size_t)j * bs];
}

// A = L L^T, in place, for the lower triangle of a tile.
__attribute__((noinline)) static void factor(int k) {
  double *t = tile(k, k);
  for (int c = 0; c < bs; c++) {
    double d = t[(size_t)c * n + c];
    for (int p = 0; p < c; p++) {
      d -= t[(size_t)c * n + p] * t[(size_t)c * n + p];
    }
    d = std::sqrt(d);
    t[(size_t)c * n + c] = d;
    for (int r = c + 1; r < bs; r++) {
      double s = t[(size_t)r * n + c];
      for (int p = 0; p < c; p++) {
        s -= t[(size_t)r * n + p] * t[(size_t)c * n + p];
      }
      t[(size_t)r * n + c] = s / d;
    }
  }
}

// X <- X L^-T, where L is the factored tile k.
__attribute__((noinline)) static void solve(int i, int k) {
  double *x = tile(i, k);
  const double *l = tile(k, k);
  for (int r = 0; r < bs; r++) {
    for (int c = 0; c < bs; c++) {
      double s = x[(size_t)r * n + c];
      for (int p = 0; p < c; p++) {
        s -= x[(size_t)r * n + p] * l[(size_t)c * n + p];
      }
      x[(size_t)r * n + c] = s / l[(size_t)c * n + c];
    }
  }
}

// A <- A - X Y^T, where X is tile (i, k) and Y is tile (j, k).
__attribute__((noinline)) static void update(int i, int j, int k) {
  double *t = tile(i, j);
  const double *x = tile(i, k);
  const double *y = tile(j, k);
  for (int r = 0; r < bs; r++) {
    for (int c = 0; c < bs; c++) {
      double s = 0.0;
      for (int p = 0; p < bs; p++) {
        s += x[(size_t)r * n + p] * y[(size_t)c * n + p];
      }
      t[(size_t)r * n + c] -= s;
    }
  }
}

static void initialize() {
  matrix.resize((size_t)n * n);
  for (int r = 0; r < n; r++) {
    for (int c = 0; c <= r; c++) {
      double v = (double)(hash((uint64_t)r * n + c) >> 11) / (double)(1ull << 53);
      matrix[(size_t)r * n + c] = matrix[(size_t)c * n + r] =
          (r == c) ? v + n : v;
    }
  }
}

static void cholesky_serial() {
  for (int k = 0; k < B; k++) {
    factor(k);
    for (int i = k + 1; i < B; i++) {
      solve(i, k);
    }
    for (int i = k + 1; i < B; i++) {
      for (int j = k + 1; j <= i; j++) {
        update(i, j, k);
      }
    }
  }
}

static void cholesky_levels() {
  for (int k = 0; k < B; k++) {
    factor(k);
    parallel_for(k + 1, B, [&](long i) { solve((int)i, k); });
    parallel_for(k + 1, B, [&](long i) {
      parallel_for(k + 1, i + 1, [&](long j) { update((int)i, (int)j, k); });
    });
  }
}

// The task that writes tile (i, j), i >= j, at step k <= j: an update if
// k < j, and at step j the factor or the solve.
static std::vector<vertex *> tasks;
static vertex *&task_of(int i, int j, int k) {
  return tasks[((size_t)i * B + j) * B + k];
}

static void cholesky_dag() {
  tasks.assign((size_t)B * B * B, nullptr);
  parallel_for(0, B, [&](long i) {
    for (int j = 0; j <= i; j++) {
      for (int k = 0; k < j; k++) {
        task_of(i, j, k) = new_vertex([=] { update((int)i, j, k); });
      }
      task_of(i, j, j) = (i == j) ? new_vertex([=] { factor(j); })
                                  : new_vertex([=] { solve((int)i, j); });
    }
  });
  // Each task gets its in-edges and is then released, while tasks released
  // earlier are running: some of the edges come too late to be needed.
  parallel_for(0, B, [&](long i) {
    for (int j = 0; j <= i; j++) {
      for (int k = 0; k <= j; k++) {
        vertex *v = task_of(i, j, k);
        if (k > 0) {
          new_edge(task_of(i, j, k - 1), v); // the last to write this tile
        }
        if (k < j) {
          new_edge(task_of(i, k, k), v); // tile (i, k) is finished
          if (i != j) {
            new_edge(task_of(j, k, k), v); // and tile (j, k)
          }
        } else if (i != j) {
          new_edge(task_of(j, j, j), v); // the factored tile j
        }
        release(v);
      }
    }
  });
  // Every task leads to the last factor.
  wait(task_of(B - 1, B - 1, B - 1));
  for (vertex *v : tasks) {
    delete v;
  }
}

// On oneTBB, as in its example: a flow graph with a node per task, which
// runs once it has heard from each of the tasks it depends on. The graph is
// built whole, by one thread, and then started at the first factor.
#ifdef BENCH_TBB
static void cholesky_tbb() {
  using message = tbb::flow::continue_msg;
  using node = tbb::flow::continue_node<message>;
  tbb::flow::graph g;
  std::vector<std::unique_ptr<node>> nodes((size_t)B * B * B);
  auto node_of = [&](int i, int j, int k) -> node & {
    return *nodes[((size_t)i * B + j) * B + k];
  };
  for (int i = 0; i < B; i++) {
    for (int j = 0; j <= i; j++) {
      for (int k = 0; k <= j; k++) {
        nodes[((size_t)i * B + j) * B + k].reset(
            new node(g, [=](const message &) {
              if (k < j) {
                update(i, j, k);
              } else if (i == j) {
                factor(j);
              } else {
                solve(i, j);
              }
            }));
      }
    }
  }
  for (int i = 0; i < B; i++) {
    for (int j = 0; j <= i; j++) {
      for (int k = 0; k <= j; k++) {
        node &v = node_of(i, j, k);
        if (k > 0) {
          tbb::flow::make_edge(node_of(i, j, k - 1), v);
        }
        if (k < j) {
          tbb::flow::make_edge(node_of(i, k, k), v);
          if (i != j) {
            tbb::flow::make_edge(node_of(j, k, k), v);
          }
        } else if (i != j) {
          tbb::flow::make_edge(node_of(j, j, j), v);
        }
      }
    }
  }
  node_of(0, 0, 0).try_put(message());
  g.wait_for_all();
}
#else
static void cholesky_tbb() {}
#endif

int main(int argc, char **argv) {
  const char *version = (argc > 1) ? argv[1] : "dag";
  n = (int)arg(argc, argv, 2, 3072);
  bs = (int)arg(argc, argv, 3, 64);
  bool check = wants_check(argc, argv, 4);
  bool known = is(version, "serial") || is(version, "levels") ||
               is(version, "dag") || is(version, "tbb");
  if (!known || (n < 1) || (bs < 1) || (n % bs != 0)) {
    std::fprintf(stderr, "usage: cholesky <serial|levels|dag|tbb> [n] [tile size] "
                         "[check]\nn must be a multiple of the tile size\n");
    return 2;
  }
  B = n / bs;
  initialize();
  bool on_tbb = is(version, "tbb");
  bool parallel = !is(version, "serial") && !on_tbb;
  auto body = [&] {
    if (is(version, "serial")) {
      cholesky_serial();
    } else if (is(version, "levels")) {
      cholesky_levels();
    } else if (on_tbb) {
      cholesky_tbb();
    } else {
      cholesky_dag();
    }
  };
  region r = on_tbb ? measure_tbb(body) : measure(parallel, body);
  double trace = 0.0;
  for (int c = 0; c < n; c++) {
    trace += matrix[(size_t)c * n + c];
  }
  std::printf("cholesky %s n=%d tile=%d\n", version, n, bs);
  std::printf("trace     %.9f\n", trace);
  report(r);
  if (check) {
    // The same tasks ran on each tile in the same order, so the factor is
    // the serial one bit for bit.
    std::vector<double> result = matrix;
    initialize();
    std::vector<double> original = matrix;
    cholesky_serial();
    bool same = true;
    for (int rr = 0; rr < n; rr++) {
      for (int c = 0; c <= rr; c++) {
        same = same && (std::memcmp(&result[(size_t)rr * n + c],
                                    &matrix[(size_t)rr * n + c],
                                    sizeof(double)) == 0);
      }
    }
    // And L L^T is the matrix we started from.
    double worst = 0.0;
    for (int rr = 0; rr < n; rr++) {
      for (int c = 0; c <= rr; c++) {
        double s = 0.0;
        for (int p = 0; p <= c; p++) {
          s += result[(size_t)rr * n + p] * result[(size_t)c * n + p];
        }
        worst = std::max(worst, std::fabs(s - original[(size_t)rr * n + c]));
      }
    }
    std::printf("residual  %.3e\n", worst);
    return report_check(same && (worst < 1e-8 * n));
  }
  return 0;
}
