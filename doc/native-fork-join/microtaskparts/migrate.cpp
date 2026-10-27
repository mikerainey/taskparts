// Shows the parent of a fork2join resuming on a different worker than the
// one that called it, and checks that results survive the move.

#include "microtaskparts.hpp"

#include <cstdio>
#include <cstdlib>

using namespace microtaskparts;

std::atomic<long> nb_joins{0};
std::atomic<long> nb_migrations{0};

// Something for a leaf to do, so that thieves have time to steal.
auto leaf(long i) -> long {
  unsigned long h = i;
  for (int k = 0; k < 200; k++) {
    h = h * 6364136223846793005ul + 1442695040888963407ul;
  }
  return (long)(h >> 60);
}

auto sum(long lo, long hi) -> long {
  if (hi - lo == 1) {
    return leaf(lo);
  }
  long mid = lo + (hi - lo) / 2;
  long a, b;
  size_t before = get_my_id();
  fork2join([&] { a = sum(lo, mid); }, [&] { b = sum(mid, hi); });
  size_t after = get_my_id();
  nb_joins.fetch_add(1, std::memory_order_relaxed);
  if (before != after) {
    nb_migrations.fetch_add(1, std::memory_order_relaxed);
  }
  return a + b;
}

int main(int argc, char **argv) {
  long n = (argc > 1) ? std::atol(argv[1]) : 1000000;
  int nb_rounds = (argc > 2) ? std::atoi(argv[2]) : 5;
  long expected = 0;
  for (long i = 0; i < n; i++) {
    expected += leaf(i);
  }
  for (int round = 0; round < nb_rounds; round++) {
    nb_joins.store(0);
    nb_migrations.store(0);
    long r = sum(0, n);
    std::printf("round %d: sum %s, %ld of %ld joins resumed on another worker\n",
                round, (r == expected) ? "ok" : "WRONG", nb_migrations.load(),
                nb_joins.load());
    if (r != expected) {
      return 1;
    }
  }
  return 0;
}
