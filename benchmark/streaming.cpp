#include "benchmark.hpp"
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <iostream>

//namespace taskparts {
  
template <typename F, typename R, typename V>
auto reduce(const F& f, const R& r, V z, size_t lo, size_t hi, size_t grain = 2) -> V {
  if (lo == hi) {
    return z;
  }
  if ((hi - lo) <= grain) {
    for (auto i = lo; i < hi; i++) {
      z = r(f(lo), z);
    }
    return z;
  }
  V rs[2];
  auto mid = (lo + hi) / 2;
  parlay::par_do([&] { rs[0] = reduce(f, r, z, lo, mid, grain); },
		 [&] { rs[1] = reduce(f, r, z, mid, hi, grain); }); 
  return r(rs[0], rs[1]); 
}

inline
uint64_t _hash(uint64_t u) {
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

auto stream(int64_t n, int64_t stream_len, int64_t grain) -> void {
  auto secs = 4.0;
  auto s = now();
  uint64_t nb_apps = 0;
  uint64_t r = 1234;
  uint64_t rounds = 0;
  while (since(s) < secs) {
    int64_t r1, r2;
    parlay::par_do([&] {
      r1 = r;
      for (auto i = 0; i < stream_len; i++) {
	r1 = _hash(r1);
      }
    }, [&] {
      r2 = reduce([&] (uint64_t i) { return _hash(i); },
		  [&] (uint64_t r1, uint64_t r2) { return _hash(r1 | r2); },
		  r, 0, n, grain);
    });
    r = _hash(r1 | r2);
    /*
    for (auto i = 0; i < stream_len; i++) {
      r = _hash(r);
      } */
    nb_apps += 2*stream_len + n;
    rounds++;
  }
  printf("nb_aps = %ld rounds = %ld r = %ld\n",nb_apps, rounds, r);
}

//} // namespace taskparts

int main(int argc, char* argv[]) {
  auto usage = "Usage: streaming <n> <stream_len> <grain>";
  if (argc != 4) { printf("%s\n", usage);
  } else {
    int64_t n;
    int64_t stream_len;
    int64_t grain;
    try { n = std::stol(argv[1]); }
    catch (...) { printf("%s\n", usage); }
    try { stream_len = std::stol(argv[2]); }
    catch (...) { printf("%s\n", usage); }
    try { grain = std::stol(argv[3]); }
    catch (...) { printf("%s\n", usage); }
    printf("n = %ld stream_len = %ld grain=%ld\n",n,stream_len,grain);
    parlay::par_do([&] { // to ensure that the taskparts scheduler is running
      taskparts::benchmark([&] {
	stream(n, stream_len, grain);
      });
    }, [&] {});

  }
  return 0;
}
