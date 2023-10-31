#include "benchmark.hpp"
#include <cstdint>
#include <cstdio>
#include <iostream>

#if defined(PARLAY_SEQUENTIAL) || defined(PARLAY_HOMEGROWN)
template <typename F1, typename F2>
auto fork2join(const F1& f1, const F2& f2) -> void {
  parlay::par_do(f1, f2);
}
#endif

namespace taskparts {
  
inline
uint64_t hash(uint64_t u) {
  uint64_t v = u * 3935559000370003845ul + 2691343689449507681ul;
  v ^= v >> 21;
  v ^= v << 37;
  v ^= v >>  4;
  v *= 4768777513237032717ul;
  v ^= v << 20;
  v ^= v >> 41;
  v ^= v <<  5;
  return v;
}

template <typename F, typename R, typename V>
auto reduce(const F& f, const R& r, V z, size_t lo, size_t hi, size_t grain = 2) -> V {
  if (lo == hi) {
    return z;
  }
  if (((lo + 1) == hi) || ((hi - lo) <= grain)) {
    for (auto i = lo; i < hi; i++) {
      z = r(f(lo), z);
    }
    return z;
  }
  V rs[2];
  auto mid = (lo + hi) / 2;
  fork2join([&] { rs[0] = reduce(f, r, z, lo, mid); },
	    [&] { rs[1] = reduce(f, r, z, mid, hi); });
  return r(rs[0], rs[1]);
}

auto oscillate(uint64_t max_n, int64_t step, size_t grain) -> void {
  auto secs = 4.0;
  auto s = now();
  uint64_t nb_apps = 0;
  uint64_t r = 1234;
  int64_t n = 1;
  printf("max n=%lu step= %d\n",max_n,step);
  while (since(s) < secs) {
    r = reduce([&] (uint64_t i) { return hash(i); },
	       [&] (uint64_t r1, uint64_t r2) { return hash(r1 | r2); },
	       r, 0, n, grain);
    nb_apps += n;
    n += step;
    if (n >= max_n) {
            printf("n=%lu\n",n);
      step *= -1l;
      n = max_n;
    } else if (n <= 1) {
                  printf("n=%lu\n",n);
      step *= -1l;
      n = 1;
    }
    
  }
  printf("nb_apps = %lu, r = %lu\n", nb_apps, r);
}

} // namespace taskparts

int main(int argc, char* argv[]) {
  auto usage = "Usage: oscillator <max_n> <step> <grain>";
  if (argc != 4) { printf("%s\n", usage);
  } else {
    uint64_t max_n;
    int64_t step;
    uint64_t grain = 2;
    try { max_n = std::stol(argv[1]); }
    catch (...) { printf("%s\n", usage); }
    try { step = std::stol(argv[2]); }
    catch (...) { printf("%s\n", usage); }
    try { grain = std::stol(argv[3]); }
    catch (...) { printf("%s\n", usage); }
    taskparts::benchmark([&] {
      taskparts::oscillate(max_n, step, grain);
    });

  }
  return 0;
}
