// Conway's game of life: a nine-point stencil over bytes, with two grids,
// each generation reading one and writing the other. The cells around the
// board stay dead.
//
// The kernel is that of the Life benchmark of Pochoir (Tang et al., SPAA
// 2011), without its periodic boundary. See stencil2d.hpp for the versions.
//
// usage: life <loops|ploops|trap|dag|tbb> [n] [generations] [width] [dt] [check]

#include "stencil2d.hpp"

#include <vector>

static int N; // cells on a side, with the edge
static std::vector<uint8_t> grids[2];

struct life {
  __attribute__((noinline)) static void row(int t, int x, int y0, int y1) {
    const uint8_t *a = grids[t & 1].data() + (size_t)x * N;
    uint8_t *b = grids[(t + 1) & 1].data() + (size_t)x * N;
    for (int y = y0; y < y1; y++) {
      int neighbors = a[y - N - 1] + a[y - N] + a[y - N + 1] + a[y - 1] +
                      a[y + 1] + a[y + N - 1] + a[y + N] + a[y + N + 1];
      b[y] = (uint8_t)((neighbors == 3) | ((neighbors == 2) & a[y]));
    }
  }
  static void initialize(int n) {
    N = n + 2;
    for (auto &g : grids) {
      g.assign((size_t)N * N, 0);
    }
    for (int x = 1; x <= n; x++) {
      for (int y = 1; y <= n; y++) {
        uint64_t h = microtaskparts::hash((uint64_t)x * N + y);
        grids[0][(size_t)x * N + y] = (uint8_t)((h % 3) == 0);
      }
    }
  }
  static double checksum(int steps) {
    double alive = 0.0;
    for (uint8_t v : grids[steps & 1]) {
      alive += v;
    }
    return alive;
  }
  static void *save(int steps) {
    void *copy = std::malloc(grids[0].size());
    std::memcpy(copy, grids[steps & 1].data(), grids[0].size());
    return copy;
  }
  static bool same(int steps, const void *other) {
    return std::memcmp(grids[steps & 1].data(), other, grids[0].size()) == 0;
  }
};

int main(int argc, char **argv) {
  return stencil::main_of<life>("life", argc, argv);
}
