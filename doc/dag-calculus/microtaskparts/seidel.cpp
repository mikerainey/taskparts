// A Gauss-Seidel heat-transfer simulation, with a five-point stencil, over
// a grid divided into square blocks. Three versions:
//
//   serial  one block after another, one time step after another;
//   planes  fork join: the blocks of a hyperplane of space and time in
//           parallel, one hyperplane after another;
//   dag     DAG calculus: a block runs a time step as soon as the blocks
//           that it depends on have run theirs.
//
// The stencil, the initial grid, and the structure of the dag version follow
// the benchmark of Acar et al. (ICFP 2016).
//
// usage: seidel <serial|planes|dag> [n] [block size] [time steps] [check]

#include "microtaskparts.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

using namespace microtaskparts;

// The grid has N = n + 2 rows and columns, of which the outermost are a
// fixed boundary. There are B x B blocks of bs x bs cells.
static int N, bs, B;
static double *grid;

// Update, in place, the block whose top-left interior cell is a[N + 1].
//
// Not inlined, so that the three versions below run the same machine code.
// Inlined into the serial loop, this kernel was compiled into code about 20%
// faster than where it was inlined elsewhere, which made the other versions
// look slower on one worker than their scheduling makes them.
__attribute__((noinline)) static void seidel_block(double *a) {
  for (int i = 1; i <= bs; i++) {
    for (int j = 1; j <= bs; j++) {
      a[i * N + j] = 0.2 * (a[i * N + j] + a[(i - 1) * N + j] +
                            a[(i + 1) * N + j] + a[i * N + j - 1] +
                            a[i * N + j + 1]);
    }
  }
}

static void run_block(int i, int j) {
  seidel_block(&grid[(size_t)N * (i * bs) + (j * bs)]);
}

static void initialize(double *a) {
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      bool hot = (i == 25 && j == 25) || (i == N - 25 && j == N - 25);
      a[(size_t)N * i + j] = hot ? 500.0 : 0.0;
    }
  }
}

static void seidel_serial(int nb_steps) {
  for (int t = 0; t < nb_steps; t++) {
    for (int i = 0; i < B; i++) {
      for (int j = 0; j < B; j++) {
        run_block(i, j);
      }
    }
  }
}

/*---------------------------------------------------------------------*/
/* Fork join: one hyperplane at a time */

template <typename F> void parallel_for(int lo, int hi, const F &f) {
  if (hi - lo <= 0) {
    return;
  }
  if (hi - lo == 1) {
    f(lo);
    return;
  }
  int mid = lo + (hi - lo) / 2;
  fork2join([&] { parallel_for(lo, mid, f); },
            [&] { parallel_for(mid, hi, f); });
}

// Block (i, j) at time step t depends on blocks (i-1, j) and (i, j-1) at
// step t, and on blocks (i+1, j) and (i, j+1) at step t-1. All four lie on
// the hyperplane i + j + 2t = h - 1, if the block itself lies on hyperplane
// h. So the blocks of a hyperplane are independent.
static void seidel_planes(int nb_steps) {
  int nb_planes = 2 * (B - 1) + 2 * (nb_steps - 1) + 1;
  for (int h = 0; h < nb_planes; h++) {
    int t_lo = std::max(0, (h - 2 * (B - 1) + 1) / 2);
    int t_hi = std::min(nb_steps - 1, h / 2);
    parallel_for(t_lo, t_hi + 1, [&](int t) {
      int s = h - 2 * t; // i + j
      int i_lo = std::max(0, s - (B - 1));
      int i_hi = std::min(B - 1, s);
      parallel_for(i_lo, i_hi + 1, [&](int i) { run_block(i, s - i); });
    });
  }
}

/*---------------------------------------------------------------------*/
/* DAG calculus: one vertex per block */

// A block is a vertex that runs once per time step. Its out-edges are
// implicit: they go to its neighbors in the grid of blocks.
struct block : vertex {
  int i, j;
  int steps_left;

  void run() override { run_block(i, j); }

  int nb_neighbors() const {
    return (i > 0) + (j > 0) + (i + 1 < B) + (j + 1 < B);
  }

  // Runs on the block's stack, after its body. The block may be made ready
  // again, and run on another stack, as soon as a neighbor is counted down.
  void notify() override {
    bool last_step = (--steps_left == 0);
    // Our next time step waits for every neighbor: for the blocks above and
    // to the left to run that step, and for the blocks below and to the
    // right to run this one. Set the count before any of them can finish.
    int n = nb_neighbors();
    pending.store((n == 0) ? 1 : n, std::memory_order_release);
    // The blocks below and to the right wait for us to run this step.
    if (i + 1 < B) {
      count_down(this + B);
    }
    if (j + 1 < B) {
      count_down(this + 1);
    }
    if (last_step) {
      if ((i + 1 == B) && (j + 1 == B)) {
        vertex::notify(); // the whole computation is done
      }
      return;
    }
    // The blocks above and to the left wait for us before their next step.
    if (i > 0) {
      count_down(this - B);
    }
    if (j > 0) {
      count_down(this - 1);
    }
    if (n == 0) {
      count_down(this); // a grid of one block
    }
  }
};

static void seidel_dag(int nb_steps) {
  std::unique_ptr<block[]> blocks(new block[(size_t)B * B]);
  parallel_for(0, B, [&](int i) {
    for (int j = 0; j < B; j++) {
      block &b = blocks[(size_t)i * B + j];
      b.i = i;
      b.j = j;
      b.steps_left = nb_steps;
      // The first step waits only for the blocks above and to the left. The
      // first block waits for its release.
      int n = (i > 0) + (j > 0);
      b.pending.store((n == 0) ? 1 : n);
    }
  });
  release(&blocks[0]);
  wait(&blocks[(size_t)B * B - 1]);
}

/*---------------------------------------------------------------------*/

int main(int argc, char **argv) {
  const char *version = (argc > 1) ? argv[1] : "dag";
  int n = (argc > 2) ? std::atoi(argv[2]) : 1024;
  bs = (argc > 3) ? std::atoi(argv[3]) : 32;
  int nb_steps = (argc > 4) ? std::atoi(argv[4]) : 20;
  bool check = (argc > 5) && (std::strcmp(argv[5], "check") == 0);
  if ((n < 1) || (bs < 1) || (n % bs != 0) || (nb_steps < 1)) {
    std::fprintf(stderr, "seidel: n must be a positive multiple of the "
                         "block size, and there must be a time step\n");
    return 2;
  }
  N = n + 2;
  B = n / bs;
  std::vector<double> data((size_t)N * N);
  grid = data.data();
  initialize(grid);
  auto start = std::chrono::steady_clock::now();
  if (std::strcmp(version, "serial") == 0) {
    seidel_serial(nb_steps);
  } else if (std::strcmp(version, "planes") == 0) {
    launch([&] { seidel_planes(nb_steps); });
  } else if (std::strcmp(version, "dag") == 0) {
    launch([&] { seidel_dag(nb_steps); });
  } else {
    std::fprintf(stderr, "seidel: unknown version %s\n", version);
    return 2;
  }
  std::chrono::duration<double> elapsed =
      std::chrono::steady_clock::now() - start;
  double sum = 0.0;
  for (double x : data) {
    sum += x;
  }
  std::printf("seidel %s n=%d block=%d steps=%d\n", version, n, bs, nb_steps);
  std::printf("checksum  %.9f\n", sum);
  std::printf("workers   %zu\n", get_nb_workers());
  std::printf("exectime  %.3f s\n", elapsed.count());
  if (check) {
    std::vector<double> reference((size_t)N * N);
    grid = reference.data();
    initialize(grid);
    seidel_serial(nb_steps);
    bool same = (std::memcmp(reference.data(), data.data(),
                             sizeof(double) * data.size()) == 0);
    std::printf("check     %s\n", same ? "ok" : "WRONG");
    return same ? 0 : 1;
  }
  return 0;
}
