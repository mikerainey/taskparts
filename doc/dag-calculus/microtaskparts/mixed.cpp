// Checks that vertices and fork join mix, in both directions: vertex bodies
// that call fork2join, branches of fork2join that build DAGs and wait for
// them, several stacks that wait for the same vertex, and a vertex that
// gains in-edges while its predecessors run.

#include "microtaskparts.hpp"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>

using namespace microtaskparts;

auto fib_serial(long n) -> long {
  return (n < 2) ? n : fib_serial(n - 1) + fib_serial(n - 2);
}

auto fib_fork(long n) -> long {
  if (n < 2) {
    return n;
  }
  long a, b;
  fork2join([&] { a = fib_fork(n - 1); }, [&] { b = fib_fork(n - 2); });
  return a + b;
}

// Fibonacci in DAG calculus: a vertex for each recursive call, and the
// caller waits for both. Below the cutoff, the vertices call fork2join.
auto fib_dag(long n, long cutoff) -> long {
  if (n <= cutoff) {
    return fib_fork(n);
  }
  long a, b;
  std::unique_ptr<vertex> va(new_vertex([&] { a = fib_dag(n - 1, cutoff); }));
  std::unique_ptr<vertex> vb(new_vertex([&] { b = fib_dag(n - 2, cutoff); }));
  release(va.get());
  release(vb.get());
  wait(va.get());
  wait(vb.get());
  return a + b;
}

// A chain of vertices, each waiting for the one before, and a group of
// branches of fork2join that all wait for the last vertex of the chain.
auto chain(long length, long nb_waiters) -> bool {
  std::vector<std::unique_ptr<vertex>> vs;
  std::atomic<long> order{0};
  std::atomic<bool> ok{true};
  for (long i = 0; i < length; i++) {
    vs.emplace_back(new_vertex([&order, &ok, i] {
      if (order.fetch_add(1) != i) {
        ok.store(false);
      }
      if (fib_fork(12) != 144) {
        ok.store(false);
      }
    }));
    if (i > 0) {
      new_edge(vs[i - 1].get(), vs[i].get());
    }
  }
  vertex *last = vs.back().get();
  auto waiters = [&](auto &&self, long lo, long hi) -> void {
    if (hi - lo == 1) {
      wait(last);
      if (order.load() != length) {
        ok.store(false);
      }
      return;
    }
    long mid = lo + (hi - lo) / 2;
    fork2join([&] { self(self, lo, mid); }, [&] { self(self, mid, hi); });
  };
  // Release the chain from inside a branch, so that ready vertices sit in
  // a deque above the second branch of a fork.
  fork2join(
      [&] {
        for (long i = length; i > 0; i--) {
          release(vs[i - 1].get());
        }
      },
      [&] { waiters(waiters, 0, nb_waiters); });
  wait(last);
  return ok.load() && (order.load() == length);
}

// Async-finish: a tree of vertices, each of which adds an edge from every
// child it spawns to one vertex, the join. So the join gains in-edges while
// its predecessors run, and their number is known only when the last one
// has finished. Adding the edges is safe because whoever adds one holds the
// join's count above zero: the caller of finish, which has yet to release
// the join, or a running vertex, which is itself a predecessor of the join.
struct spawned {
  vertex *v;
  spawned *next;
};

auto async(vertex *join, std::atomic<spawned *> &all, std::atomic<long> &count,
           int depth) -> void {
  vertex *v = new_vertex([join, &all, &count, depth] {
    count.fetch_add(1);
    if (depth > 1) {
      async(join, all, count, depth - 1);
      async(join, all, count, depth - 1);
    }
  });
  auto s = new spawned{v, all.load()};
  while (!all.compare_exchange_weak(s->next, s)) {
  }
  new_edge(v, join);
  release(v);
}

auto finish(int depth) -> bool {
  std::atomic<spawned *> all{nullptr};
  std::atomic<long> count{0};
  long seen_by_join = -1;
  std::unique_ptr<vertex> join(
      new_vertex([&] { seen_by_join = count.load(); }));
  async(join.get(), all, count, depth);
  release(join.get());
  wait(join.get());
  for (spawned *s = all.load(); s != nullptr;) {
    spawned *next = s->next;
    delete s->v;
    delete s;
    s = next;
  }
  long expected = (1l << depth) - 1;
  return (seen_by_join == expected) && (count.load() == expected);
}

int main(int argc, char **argv) {
  long n = (argc > 1) ? std::atol(argv[1]) : 27;
  int nb_rounds = (argc > 2) ? std::atoi(argv[2]) : 5;
  long expected = fib_serial(n);
  bool ok = true;
  for (int round = 0; round < nb_rounds; round++) {
    long r1 = 0, r2 = 0, r3 = 0;
    bool c = false, f1 = false, f2 = false;
    launch([&] {
      r1 = fib_dag(n, n - 12);
      fork2join([&] { r2 = fib_dag(n, n - 10); },
                [&] { r3 = fib_dag(n - 1, n - 9); });
      c = chain(200, 64);
      fork2join([&] { f1 = finish(12); }, [&] { f2 = finish(10); });
    });
    bool round_ok = (r1 == expected) && (r2 == expected) &&
                    (r3 == fib_serial(n - 1)) && c && f1 && f2;
    std::printf("round %d: %s\n", round, round_ok ? "ok" : "WRONG");
    ok = ok && round_ok;
  }
  return ok ? 0 : 1;
}
