// The length of the longest common subsequence of two strings, by dynamic
// programming over a table divided into square blocks. A block reads the
// row above it and the column to its left, so it depends on the block above
// and the block to the left: a wavefront. Four versions:
//
//   serial  one block after another, row by row;
//   planes  fork join: the blocks of an antidiagonal in parallel, one
//           antidiagonal after another;
//   dc      fork join, by divide and conquer: the top-left quarter of the
//           blocks, then the top-right and bottom-left in parallel, then
//           the bottom-right, recursively;
//   dag     DAG calculus: a block is a vertex that waits for the block
//           above and the block to the left, and for nothing else.
//
// LCS is one of the benchmarks of Pochoir (Tang et al., SPAA 2011), which
// runs it as a stencil over antidiagonals. The dag version is the wavefront
// that Intel's guide to Threading Building Blocks gives as a design pattern,
// with a count of unfinished predecessors in each block; the dc version is
// the usual one in the literature on Cilk.
//
// Only the edges of blocks are kept, not the table: one row of the table
// across all columns, and for each row of blocks one column.
//
// usage: lcs <serial|planes|dc|dag> [n] [block size] [check]

#include "harness.hpp"

#include <memory>
#include <vector>

using namespace bench;

static int n;      // the length of both strings
static int bs, B;  // block size, and blocks on a side
static std::vector<uint8_t> a, b;
// top[c], 0 <= c <= n: the table's entry in column c of the last row that
// the blocks of that column have computed.
static std::vector<uint32_t> top;
// left[I][k], 0 <= k <= rows of blocks I: the entry in row first(I) - 1 + k
// of the last column that the blocks of row I have computed.
static std::vector<std::vector<uint32_t>> left;

static int first(int I) { return I * bs + 1; }               // row or column
static int last(int I) { return std::min(n, (I + 1) * bs); } // inclusive

__attribute__((noinline)) static void run_block(int I, int J) {
  int r0 = first(I), r1 = last(I), c0 = first(J), c1 = last(J);
  uint32_t *t = &top[c0];
  uint32_t *l = left[I].data();
  const uint8_t *bj = &b[c0 - 1];
  int nb_cols = c1 - c0 + 1;
  // The entry of the last column in the row above the current one.
  uint32_t right = top[c1];
  for (int k = 1; k <= r1 - r0 + 1; k++) {
    uint32_t diagonal = l[k - 1], west = l[k];
    l[k - 1] = right;
    uint8_t ai = a[r0 + k - 2];
    for (int x = 0; x < nb_cols; x++) {
      uint32_t north = t[x];
      uint32_t v = (ai == bj[x]) ? diagonal + 1 : std::max(north, west);
      diagonal = north;
      t[x] = v;
      west = v;
    }
    right = west;
  }
  l[r1 - r0 + 1] = right;
}

static void initialize() {
  top.assign(n + 1, 0);
  left.resize(B);
  for (int I = 0; I < B; I++) {
    left[I].assign(last(I) - first(I) + 2, 0);
  }
}

static void lcs_serial() {
  for (int I = 0; I < B; I++) {
    for (int J = 0; J < B; J++) {
      run_block(I, J);
    }
  }
}

static void lcs_planes() {
  for (int d = 0; d <= 2 * (B - 1); d++) {
    int lo = std::max(0, d - (B - 1)), hi = std::min(B - 1, d);
    parallel_for(lo, hi + 1, [&](long I) { run_block((int)I, d - (int)I); });
  }
}

// Blocks I0 <= I < I1, J0 <= J < J1.
static void lcs_dc(int I0, int I1, int J0, int J1) {
  if ((I1 - I0 == 1) && (J1 - J0 == 1)) {
    run_block(I0, J0);
    return;
  }
  int Im = I0 + (I1 - I0) / 2, Jm = J0 + (J1 - J0) / 2;
  if (I1 - I0 == 1) {
    lcs_dc(I0, I1, J0, Jm);
    lcs_dc(I0, I1, Jm, J1);
  } else if (J1 - J0 == 1) {
    lcs_dc(I0, Im, J0, J1);
    lcs_dc(Im, I1, J0, J1);
  } else {
    lcs_dc(I0, Im, J0, Jm);
    fork2join([&] { lcs_dc(I0, Im, Jm, J1); }, [&] { lcs_dc(Im, I1, J0, Jm); });
    lcs_dc(Im, I1, Jm, J1);
  }
}

// The out-edges of a block are implicit: they go to the block below and to
// the block to the right.
struct block : vertex {
  int I, J;

  void run() override { run_block(I, J); }

  // The last count_down may let the computation finish, and the blocks be
  // freed, so this reads the block's fields first.
  void notify() override {
    const bool below = (I + 1 < B), beside = (J + 1 < B);
    block *self = this;
    if (below) {
      count_down(self + B);
    }
    if (beside) {
      count_down(self + 1);
    }
    if (!below && !beside) {
      vertex::notify(); // the last block: whoever waits for it
    }
  }
};

static void lcs_dag() {
  std::unique_ptr<block[]> blocks(new block[(size_t)B * B]);
  parallel_for(0, B, [&](long I) {
    for (int J = 0; J < B; J++) {
      block &v = blocks[(size_t)I * B + J];
      v.I = (int)I;
      v.J = J;
      int nb = (I > 0) + (J > 0);
      v.pending.store((nb == 0) ? 1 : nb); // the first waits for its release
    }
  });
  release(&blocks[0]);
  wait(&blocks[(size_t)B * B - 1]);
}

int main(int argc, char **argv) {
  const char *version = (argc > 1) ? argv[1] : "dag";
  n = (int)arg(argc, argv, 2, 60000);
  bs = (int)arg(argc, argv, 3, 512);
  bool check = wants_check(argc, argv, 4);
  bool known = is(version, "serial") || is(version, "planes") ||
               is(version, "dc") || is(version, "dag");
  if (!known || (n < 1) || (bs < 1)) {
    std::fprintf(stderr,
                 "usage: lcs <serial|planes|dc|dag> [n] [block size] [check]\n");
    return 2;
  }
  B = (n + bs - 1) / bs;
  // Two strings over four letters; the second is the first with one letter
  // in four redrawn.
  a.resize(n);
  b.resize(n);
  for (int i = 0; i < n; i++) {
    a[i] = (uint8_t)(hash(i) & 3);
    uint64_t h = hash((uint64_t)i + ((uint64_t)1 << 40));
    b[i] = ((h & 3) == 0) ? (uint8_t)((h >> 2) & 3) : a[i];
  }
  initialize();
  bool parallel = !is(version, "serial");
  region r = measure(parallel, [&] {
    if (is(version, "serial")) {
      lcs_serial();
    } else if (is(version, "planes")) {
      lcs_planes();
    } else if (is(version, "dc")) {
      lcs_dc(0, B, 0, B);
    } else {
      lcs_dag();
    }
  });
  std::vector<uint32_t> result = top;
  std::printf("lcs %s n=%d block=%d\n", version, n, bs);
  std::printf("length    %u\n", result[n]);
  report(r);
  if (check) {
    initialize();
    lcs_serial();
    return report_check(result == top);
  }
  return 0;
}
