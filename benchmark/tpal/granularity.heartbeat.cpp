#include "rollforward.h"
#include <cstdint>
#include <cstdlib>
#include <stdio.h>
#include <assert.h>
#include <functional>

#include "taskparts/cmdline.hpp"
#include "taskparts/hash.hpp"
#include "taskparts/nativeforkjoin.hpp"
#include "taskparts/posix/perworkerid.hpp"
#include <taskparts/benchmark.hpp>

#define D 64

extern
void nb_occurrences_handler(void* lo, void* hi, uint64_t r, int64_t* dst, bool& promoted);

void rollforward_handler_annotation __rf_handle_nb_occurrences_heartbeat(void* lo, void* hi,
									 uint64_t r, int64_t* dst, bool& promoted) {
  nb_occurrences_handler(lo, hi, r, dst, promoted);
  rollbackward
}

template <typename T, typename P>
auto nb_occurrences_heartbeat_loop(T* lo, T* hi, const P& p,
				   uint64_t n, int64_t* dst) {
  if (lo == hi) {
    goto exit;
  }
  for (; ; ) {
    auto lo2 = lo;
    auto hi2 = std::min(lo + D, hi);
    for (; lo2 < hi2; lo2++) {
      n += p(lo2) ? 1 : 0;
    }
    lo = lo2;
    if (lo == hi) {
      break;
    }
    bool promoted = false;
    __rf_handle_nb_occurrences_heartbeat(lo, hi, n, dst, promoted);
    if (rollforward_branch_unlikely(promoted)) {
      return;
    }
  }
 exit:
  *dst = n;
}

std::function<void(void*, void*, uint64_t, int64_t*)> callback;

void nb_occurrences_heartbeat(void* lo, void* hi, uint64_t r, int64_t* dst) {
  callback(lo, hi, r, dst);
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
    write_random_chars(lo2, lo2 + sizeof(T));
  }
}
  
template <class T>
T* create_random_array(int n) {
  T* data = (T*)malloc(sizeof(T) * n);
  write_random_data(data, data + n);
  return data;
}

template <typename T, typename P>
auto using_item_type(const P& p) {
  auto nszb = taskparts::cmdline::parse_or_default_long("n", 1l << 29);
  auto szb = sizeof(T);
  auto n = nszb / szb;
  assert(cilk_heuristic::nworkers != -1);
  printf("nb_items %lu\n",n);
  T* data;
  callback = [&] (void* lo, void* hi, uint64_t r, int64_t* dst) -> void {
    nb_occurrences_heartbeat_loop((T*)lo, (T*)hi, p, r, dst);
  };
  taskparts::benchmark_nativeforkjoin([&] (auto sched) { // benchmark
    callback(data, data + n, 0, &answer);
  }, [&] (auto sched) { // setup
    data = create_random_array<T>(n);
  }, [&] (auto sched) { // teardown
    printf("result %lu\n",answer);
  }, [&] (auto sched) { // reset
    free(data);
  });
}

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

auto select_item_type() -> void {
  auto szb = taskparts::cmdline::parse_or_default_int("item_szb", 1);
  static constexpr
  auto szb1 = 1;
  static constexpr
  auto szb2 = 1<<4;
  static constexpr
  auto szb3 = 1<<11;
  static constexpr
  auto szb4 = 1<<17; // 131072
  static constexpr
  auto szb5 = 1<<23; // 8388608
  if (szb == szb1) {
    using_item_type<char>([] (char* c) { return *c == 'a'; });
  } else if (szb == szb2) {
    using_item_type<bytes<szb2>>([] (auto* b) { return (*((uint64_t*)b) / std::max<uint64_t>(1, *(((uint64_t*)b)+1))) == 123; });
  } else if (szb == szb3) {
    using_item_type<bytes<szb3>>([] (auto* b) { return hash(*b) == 2017L; });
  } else if (szb == szb4) {
    using_item_type<bytes<szb4>>([] (auto* b) { return hash(*b) == 2017L; });
  } else if (szb == szb5) {
    using_item_type<bytes<szb5>>([] (auto* b) { return hash(*b) == 2017L; });
  } else {
    std::cerr << "bogus szb " <<  szb << std::endl;
    exit(0);
  }
}
