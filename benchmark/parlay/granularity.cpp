#include <cstdint>
#include <cstdlib>
#include <stdio.h>
#include <assert.h>

#include "../../example/fib_manualfiber.hpp"
#include "taskparts/cmdline.hpp"
#include "taskparts/hash.hpp"
#include "taskparts/nativeforkjoin.hpp"
#include "taskparts/posix/perworkerid.hpp"
#include <taskparts/benchmark.hpp>

namespace serial {
  template <typename T, typename P>
  auto nb_occurrences(T* lo, T* hi, const P& p) -> uint64_t {
    uint64_t n = 0;
    for (auto i = lo; i != hi; i++) {
      n += p(i) ? 1 : 0;
    }
    return n;
  }
}

using taskparts_scheduler = taskparts::bench_scheduler;

namespace manual_gc {
  uint64_t threshold = 2048;
  template <typename T, typename P>
  auto nb_occurrences(T* lo, T* hi, const P& p) -> uint64_t {
    auto n = hi - lo;
    if (n <= threshold) {
      return serial::nb_occurrences(lo, hi, p);
    }
    auto mid = lo + (n / 2);
    int64_t r1, r2;
    auto lf = [&] {
      r1 = nb_occurrences(lo, mid, p);
    };
    auto rf = [&] {
      r2 = nb_occurrences(mid, hi, p);
    };
    taskparts::fork2join<decltype(lf), decltype(rf), taskparts_scheduler>(lf, rf);
    return r1 + r2;
  }
}

namespace cilk_heuristic {
  int nworkers = -1;
  auto ceiling_div(uint64_t x, uint64_t y) -> uint64_t {
    return (x + y - 1) / y;
  }
  template <typename T, typename P>
  auto nb_occurrences_rec(T* lo, T* hi, const P& p, uint64_t grainsize) -> uint64_t {
    auto n = hi - lo;
    if (n <= grainsize) {
      return serial::nb_occurrences(lo, hi, p);
    }
    auto mid = lo + (n / 2);
    int64_t r1, r2;
    auto lf = [&] {
      r1 = nb_occurrences_rec(lo, mid, p, grainsize);
    };
    auto rf = [&] {
      r2 = nb_occurrences_rec(mid, hi, p, grainsize);
    };
    taskparts::fork2join<decltype(lf), decltype(rf), taskparts_scheduler>(lf, rf);
    return r1 + r2;
  }
  template <typename T, typename P>
  auto nb_occurrences(T* lo, T* hi, const P& p) -> uint64_t {
    uint64_t grainsize = std::min<uint64_t>(2048, ceiling_div(hi - lo, 8 * nworkers));
    return nb_occurrences_rec(lo, hi, p, grainsize);
  }
}

int64_t answer = -1;

void write_random_chars(char* lo, char* hi) {
  for (int i = 0; lo != hi; lo++, i++) {
    union {
      char* p;
      uint64_t v;
    } x;
    x.p = lo;
    *lo = (char)taskparts::hash((unsigned int)x.v);
  }
}
  
template <class T>
void write_random_data(T* lo, T* hi) {
  for (auto it = lo; it != hi; it++) {
    auto lo2 = (char*)it;
    auto hi2 = lo2 + sizeof(T);
    write_random_chars(lo2, hi2);
  }
}
  
template <class T>
T* create_random_array(int n) {
  T* data = (T*)malloc(sizeof(T) * n);
  auto lo = data;
  auto hi = data + n;
  write_random_data(lo, hi);
  return data;
}

template <typename T, typename P>
auto using_item_type(const P& p) {
  auto n = taskparts::cmdline::parse_or_default_long("n", 1l << 29);
  T* data;
  taskparts::cmdline::dispatcher d;
  d.add("serial", [&] {
    answer = serial::nb_occurrences(data, data + n, p);
  });
  d.add("manual", [&] {
    answer = manual_gc::nb_occurrences(data, data + n, p);
  });
  d.add("cilk", [&] {
    assert(cilk_heuristic::nworkers != -1);
    answer = cilk_heuristic::nb_occurrences(data, data + n, p);
  });
  taskparts::benchmark_nativeforkjoin([&] (auto sched) { // benchmark
    d.dispatch_or_default("method", "serial");
  }, [&] (auto sched) { // setup
    data = create_random_array<T>(n);
  }, [&] (auto sched) { // teardown
    printf("result %lu\n",answer);
  }, [&] (auto sched) { // reset
    free(data);
  });
}

static constexpr
int szb0 = 64;
static constexpr
int szb6 = 128;
static constexpr
int szb5 = 256;
static constexpr
int szb1 = 4 * 256;
static constexpr
int szb2 = 2*szb1;
static constexpr
int szb3 = 1 << 17;
static constexpr
int szb4 = 1 << 20;

template <int szb>
class bytes {
public:
  char b[szb];
};

template <int szb>
auto hash(bytes<szb>& b) -> uint64_t {
  uint64_t h = 0;
  auto lo = (uint64_t*)&b.b[0];
  auto hi = lo + (szb / sizeof(uint64_t));
  for (auto it = lo; it != hi; it++) {
    h = taskparts::hash(*it + h);
  }
  return h;
}

auto select_item_type() {
  auto szb = taskparts::cmdline::parse_or_default_int("item_szb", 1);
  if (szb == 1) {
    using_item_type<char>([] (char* c) { return *c == 'a'; });
    return;
  }
  if (szb == szb0) {
    using_item_type<bytes<szb0>>([] (bytes<szb0>* b) { return hash(*b) == 2017L; });
  } else {
    std::cerr << "bogus szb " <<  szb << std::endl;
    exit(0);
  }
}

int main() {
  manual_gc::threshold = taskparts::cmdline::parse_or_default_long("threshold", manual_gc::threshold);
  cilk_heuristic::nworkers = taskparts::perworker::nb_workers();
  select_item_type();
  return 0;
}
