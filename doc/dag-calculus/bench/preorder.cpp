// Evaluate a random directed acyclic graph of small matrix operations: each
// cell is a matrix that is given, or the sum, difference or product of two
// earlier cells. A cell can be computed once its inputs are. Three versions:
//
//   serial  the cells in the order of their numbers, which is a topological
//           order;
//   levels  fork join: a cell's level is one more than the highest level of
//           its inputs, and the cells of a level run in parallel, one level
//           after another;
//   dag     DAG calculus: a cell is a vertex whose count is the number of
//           its inputs, and which counts down the cells that read it.
//
// This is the parallel_preorder example of Intel's Threading Building
// Blocks, where each cell holds a count of the inputs it still waits for,
// and a cell that finishes feeds the cells whose count it brings to zero to
// parallel_do. The matrices here hold integers that wrap around, in place
// of floating-point numbers, so that the result is exact.
//
// The cells are small (a product of two 16 x 16 matrices), so this measures
// what a vertex and an edge cost more than the other benchmarks do.
//
// usage: preorder <serial|levels|dag|tbb> [cells] [check]

#include "harness.hpp"

#include <memory>
#include <vector>

using namespace bench;

static constexpr int order = 16;
struct value {
  uint32_t x[order][order];
};

enum operation { given, add, subtract, multiply };

static int nb_cells;
static std::vector<operation> ops;
static std::vector<int> inputs;          // two per cell
static std::vector<int> readers_start;   // nb_cells + 1 offsets into readers
static std::vector<int> readers;         // the cells that read each cell
static std::vector<std::vector<int>> levels;
static std::vector<value> values;

__attribute__((noinline)) static void compute(int c) {
  value &v = values[c];
  if (ops[c] == given) {
    uint64_t h = hash((uint64_t)c);
    for (int i = 0; i < order; i++) {
      for (int j = 0; j < order; j++) {
        h = hash(h);
        v.x[i][j] = (uint32_t)h;
      }
    }
    return;
  }
  const value &p = values[inputs[2 * c]], &q = values[inputs[2 * c + 1]];
  for (int i = 0; i < order; i++) {
    for (int j = 0; j < order; j++) {
      if (ops[c] == add) {
        v.x[i][j] = p.x[i][j] + q.x[i][j];
      } else if (ops[c] == subtract) {
        v.x[i][j] = p.x[i][j] - q.x[i][j];
      } else {
        uint32_t s = 0;
        for (int k = 0; k < order; k++) {
          s += p.x[i][k] * q.x[k][j];
        }
        v.x[i][j] = s;
      }
    }
  }
}

// One cell in eight is given, and so are the first few; of the others, half
// are products. The inputs of a cell are drawn from all earlier cells.
static void build_graph() {
  ops.resize(nb_cells);
  inputs.assign(2 * (size_t)nb_cells, -1);
  std::vector<int> level(nb_cells, 0), nb_readers(nb_cells, 0);
  for (int c = 0; c < nb_cells; c++) {
    uint64_t h = hash((uint64_t)c + ((uint64_t)1 << 32));
    if ((c < 4) || ((h & 7) == 0)) {
      ops[c] = given;
    } else {
      unsigned kind = (h >> 3) & 3;
      ops[c] = (kind == 0) ? add : ((kind == 1) ? subtract : multiply);
      for (int k = 0; k < 2; k++) {
        int in = (int)(hash(h + k) % (uint64_t)c);
        inputs[2 * c + k] = in;
        nb_readers[in]++;
        level[c] = std::max(level[c], level[in] + 1);
      }
    }
    if ((int)levels.size() <= level[c]) {
      levels.resize(level[c] + 1);
    }
    levels[level[c]].push_back(c);
  }
  readers_start.assign(nb_cells + 1, 0);
  for (int c = 0; c < nb_cells; c++) {
    readers_start[c + 1] = readers_start[c] + nb_readers[c];
  }
  readers.resize(readers_start[nb_cells]);
  std::vector<int> next(readers_start.begin(), readers_start.end() - 1);
  for (int c = 0; c < nb_cells; c++) {
    for (int k = 0; (k < 2) && (ops[c] != given); k++) {
      readers[next[inputs[2 * c + k]]++] = c;
    }
  }
}

static void preorder_serial() {
  for (int c = 0; c < nb_cells; c++) {
    compute(c);
  }
}

static void preorder_levels() {
  for (const std::vector<int> &level : levels) {
    parallel_for(0, (long)level.size(), [&](long k) { compute(level[k]); });
  }
}

struct cell;
static cell *cells;
// Runs once every cell that nothing reads has run.
static vertex *sink;

// The out-edges of a cell are the graph's: they go to the cells that read
// it, or to the sink if there is none.
struct cell : vertex {
  int c;

  void run() override { compute(c); }

  // The last count_down may let the computation finish, and the cells be
  // freed, so nothing here reads the cell after its first lines.
  void notify() override {
    const int *r = readers.data() + readers_start[c];
    const int *end = readers.data() + readers_start[c + 1];
    cell *all = cells;
    vertex *s = sink;
    if (r == end) {
      count_down(s);
      return;
    }
    for (; r != end; r++) {
      count_down(all + *r);
    }
  }
};

static void preorder_dag() {
  std::unique_ptr<cell[]> owned(new cell[nb_cells]);
  cells = owned.get();
  sink = new_vertex([] {});
  std::atomic<int> nb_unread{0};
  parallel_for(0, (nb_cells + 1023) / 1024, [&](long g) {
    int unread = 0;
    int hi = std::min(nb_cells, (int)(g + 1) * 1024);
    for (int c = (int)g * 1024; c < hi; c++) {
      cells[c].c = c;
      // A given cell waits for its release, and the others for two inputs.
      cells[c].pending.store((ops[c] == given) ? 1 : 2);
      unread += (readers_start[c] == readers_start[c + 1]);
    }
    nb_unread.fetch_add(unread);
  });
  sink->pending.store(nb_unread.load());
  parallel_for(0, (nb_cells + 1023) / 1024, [&](long g) {
    int hi = std::min(nb_cells, (int)(g + 1) * 1024);
    for (int c = (int)g * 1024; c < hi; c++) {
      if (ops[c] == given) {
        release(&cells[c]);
      }
    }
  });
  wait(sink);
  delete sink;
}

// On oneTBB, as in its example: parallel_for_each over the given cells,
// which are the cells of level 0, with each cell feeding the loop the
// readers whose count it brings to zero.
#ifdef BENCH_TBB
static void preorder_tbb() {
  std::unique_ptr<std::atomic<int>[]> waiting(new std::atomic<int>[nb_cells]);
  tbb::parallel_for(0, nb_cells, [&](int c) {
    waiting[c].store((ops[c] == given) ? 0 : 2, std::memory_order_relaxed);
  });
  const std::vector<int> &roots = levels[0];
  tbb::parallel_for_each(roots.begin(), roots.end(),
                         [&](int c, tbb::feeder<int> &feeder) {
                           compute(c);
                           for (int k = readers_start[c];
                                k < readers_start[c + 1]; k++) {
                             int r = readers[k];
                             if (waiting[r].fetch_sub(1) == 1) {
                               feeder.add(r);
                             }
                           }
                         });
}
#else
static void preorder_tbb() {}
#endif

int main(int argc, char **argv) {
  const char *version = (argc > 1) ? argv[1] : "dag";
  nb_cells = (int)arg(argc, argv, 2, 1000000);
  bool check = wants_check(argc, argv, 3);
  bool known = is(version, "serial") || is(version, "levels") ||
               is(version, "dag") || is(version, "tbb");
  if (!known || (nb_cells < 1)) {
    std::fprintf(stderr, "usage: preorder <serial|levels|dag|tbb> [cells] [check]\n");
    return 2;
  }
  build_graph();
  values.resize(nb_cells);
  bool on_tbb = is(version, "tbb");
  bool parallel = !is(version, "serial") && !on_tbb;
  auto body = [&] {
    if (is(version, "serial")) {
      preorder_serial();
    } else if (is(version, "levels")) {
      preorder_levels();
    } else if (on_tbb) {
      preorder_tbb();
    } else {
      preorder_dag();
    }
  };
  region r = on_tbb ? measure_tbb(body) : measure(parallel, body);
  auto digest = [&] {
    uint64_t d = 0;
    for (const value &v : values) {
      for (int i = 0; i < order; i++) {
        for (int j = 0; j < order; j++) {
          d = d * 1099511628211ul + v.x[i][j];
        }
      }
    }
    return d;
  };
  uint64_t result = digest();
  std::printf("preorder %s cells=%d levels=%zu\n", version, nb_cells,
              levels.size());
  std::printf("digest    %016llx\n", (unsigned long long)result);
  report(r);
  if (check) {
    values.assign(nb_cells, value{});
    preorder_serial();
    return report_check(result == digest());
  }
  return 0;
}
