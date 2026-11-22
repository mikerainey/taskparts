// Heat diffusion on a square plate: a five-point stencil over doubles, with
// two grids, each time step reading one and writing the other. The edge of
// the plate is held at zero.
//
// The kernel is that of the Heat benchmark of Pochoir (Tang et al., SPAA
// 2011), without its periodic boundary. See stencil2d.hpp for the versions.
//
// usage: heat <loops|ploops|trap|dag|tbb> [n] [time steps] [width] [dt] [check]

#include "stencil2d.hpp"

#include <vector>

static int N; // cells on a side, with the edge
static std::vector<double> grids[2];

struct heat {
  __attribute__((noinline)) static void row(int t, int x, int y0, int y1) {
    const double *a = grids[t & 1].data() + (size_t)x * N;
    double *b = grids[(t + 1) & 1].data() + (size_t)x * N;
    for (int y = y0; y < y1; y++) {
      b[y] = a[y] + 0.125 * (a[y + N] - 2.0 * a[y] + a[y - N]) +
             0.125 * (a[y + 1] - 2.0 * a[y] + a[y - 1]);
    }
  }
  static void initialize(int n) {
    N = n + 2;
    for (auto &g : grids) {
      g.assign((size_t)N * N, 0.0);
    }
    // A few hot squares.
    for (int k = 0; k < 5; k++) {
      int cx = 1 + (int)(microtaskparts::hash(k) % n);
      int cy = 1 + (int)(microtaskparts::hash(k + 100) % n);
      for (int x = cx; (x < cx + n / 10 + 1) && (x <= n); x++) {
        for (int y = cy; (y < cy + n / 10 + 1) && (y <= n); y++) {
          grids[0][(size_t)x * N + y] = 100.0 * (k + 1);
        }
      }
    }
  }
  static double checksum(int steps) {
    double sum = 0.0;
    for (double v : grids[steps & 1]) {
      sum += v;
    }
    return sum;
  }
  static void *save(int steps) {
    size_t szb = sizeof(double) * grids[0].size();
    void *copy = std::malloc(szb);
    std::memcpy(copy, grids[steps & 1].data(), szb);
    return copy;
  }
  static bool same(int steps, const void *other) {
    return std::memcmp(grids[steps & 1].data(), other,
                       sizeof(double) * grids[0].size()) == 0;
  }
};

int main(int argc, char **argv) {
  return stencil::main_of<heat>("heat", argc, argv);
}
