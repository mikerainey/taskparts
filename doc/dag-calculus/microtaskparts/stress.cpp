// Adversarial tests of the scheduler. Each test aims at one claim of the
// safety argument of ../main.tex (Section "Why it is still safe"), and builds
// the situation in which that claim would fail if it were false.
//
//   stress [rounds] [seed]
//
// Every test checks its own result, and a watchdog turns a test that never
// ends (a task that was lost, or a stack that was never resumed) into a
// failure. Meant to be run in the builds of the Makefile's "Builds for
// testing", on several numbers of workers; see test.sh.

#include "microtaskparts.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <thread>
#include <vector>

using namespace microtaskparts;

/*---------------------------------------------------------------------*/
/* Helpers */

// A random number that depends only on its arguments, so that a test does
// the same thing on every run with the same seed, whatever the schedule.
static uint64_t rnd(uint64_t seed, uint64_t a, uint64_t b = 0) {
  return hash(seed ^ hash(a * 0x9e3779b97f4a7c15ull + b));
}

static long fib_fork(long n) {
  if (n < 2) {
    return n;
  }
  long a, b;
  fork2join([&] { a = fib_fork(n - 1); }, [&] { b = fib_fork(n - 2); });
  return a + b;
}

// Run body(lo), ..., body(hi-1) in parallel, by forking.
template <typename Body> static void parallel_for(long lo, long hi, Body &body) {
  if (hi - lo <= 1) {
    if (hi > lo) {
      body(lo);
    }
    return;
  }
  long mid = lo + (hi - lo) / 2;
  fork2join([&] { parallel_for(lo, mid, body); },
            [&] { parallel_for(mid, hi, body); });
}

// Vertices created while a test runs, to be freed when it is over.
struct made {
  vertex *v;
  made *next;
};

static void remember(std::atomic<made *> &all, vertex *v) {
  auto m = new made{v, all.load()};
  while (!all.compare_exchange_weak(m->next, m)) {
  }
}

static long free_all(std::atomic<made *> &all) {
  long n = 0;
  for (made *m = all.exchange(nullptr); m != nullptr;) {
    made *next = m->next;
    delete m->v;
    delete m;
    m = next;
    n++;
  }
  return n;
}

/*---------------------------------------------------------------------*/
/* Test 1: a fork whose first branch waits */

// Claim: a fork that does not pop its own task may suspend at its join,
// because the task will run anyway; and no task is left behind in a deque.
//
// The first branch of every fork waits for a vertex while the fork's own
// task is still in the deque. So the stack is suspended with tasks of its
// forks in a deque, may be resumed by another worker, and then pops from a
// deque that never held its task. The second branches count themselves:
// each must run exactly once.
static void waiting_forks(int depth, std::atomic<long> &nb_seconds,
                          std::atomic<long> &sum) {
  if (depth == 0) {
    return;
  }
  fork2join(
      [&] {
        long r = 0;
        std::unique_ptr<vertex> v(new_vertex([&] { r = fib_fork(8); }));
        release(v.get());
        wait(v.get());
        sum.fetch_add(r);
        waiting_forks(depth - 1, nb_seconds, sum);
      },
      [&] {
        nb_seconds.fetch_add(1);
        waiting_forks(depth - 1, nb_seconds, sum);
      });
}

static bool test_waiting_forks(uint64_t) {
  const int depth = 9;
  std::atomic<long> nb_seconds{0}, sum{0};
  waiting_forks(depth, nb_seconds, sum);
  long nb_forks = (1l << depth) - 1;
  return (nb_seconds.load() == nb_forks) && (sum.load() == nb_forks * 21);
}

/*---------------------------------------------------------------------*/
/* Test 2: waiting for a vertex as it finishes */

// Claim: adding an edge to a vertex and finishing that vertex are settled by
// one compare-and-swap, and a suspension is released only after its stack is
// saved.
//
// The vertex has an empty body, and is released while a group of stacks is
// about to wait for it, so that the two events race each time. A waiter must
// find that the vertex has run, whichever way the race went. The vertex is
// freed as soon as the last wait has returned: the scheduler must not touch
// it after it has notified.
static bool test_wait_races(uint64_t seed) {
  bool ok = true;
  for (int i = 0; i < 300; i++) {
    long nb_waiters = 1 + rnd(seed, i) % 6;
    std::atomic<bool> ran{false};
    std::atomic<long> nb_ok{0};
    vertex *v = new_vertex([&] { ran.store(true); });
    auto waiter = [&](long) {
      wait(v);
      if (ran.load()) {
        nb_ok.fetch_add(1);
      }
    };
    bool release_first = (rnd(seed, i, 1) % 2) == 0;
    auto releaser = [&] { release(v); };
    auto waiters = [&] { parallel_for(0, nb_waiters, waiter); };
    if (release_first) {
      fork2join(releaser, waiters);
    } else {
      fork2join(waiters, releaser);
    }
    wait(v); // has finished: returns at once
    delete v;
    ok = ok && (nb_ok.load() == nb_waiters);
  }
  // The same, with a vertex that also has ordinary successors. The waiter
  // frees the vertex the moment its wait returns, which may be while the
  // vertex is still counting its other successors down.
  for (int i = 0; i < 300; i++) {
    long nb_targets = rnd(seed, i, 2) % 8;
    std::atomic<long> nb_ran{0};
    vertex *v = new_vertex([] {});
    std::unique_ptr<vertex> end(new_vertex([] {}));
    std::vector<std::unique_ptr<vertex>> targets;
    for (long k = 0; k < nb_targets; k++) {
      targets.emplace_back(new_vertex([&] { nb_ran.fetch_add(1); }));
      new_edge(v, targets.back().get());
      new_edge(targets.back().get(), end.get());
      release(targets.back().get());
    }
    release(end.get());
    release(v);
    wait(v);
    delete v;
    wait(end.get());
    ok = ok && (nb_ran.load() == nb_targets);
  }
  return ok;
}

/*---------------------------------------------------------------------*/
/* Test 3: many suspended stacks */

// Claim: no two tasks run on the same stack, and stacks pass safely from
// worker to worker.
//
// Several hundred vertices each suspend on one gate, so each holds a stack.
// When the gate opens they all finish, and more stacks come back than a
// worker's cache holds. Each vertex keeps a pattern in an array on its stack
// across the wait, and checks it afterwards.
static bool test_many_stacks(uint64_t seed) {
  const long n = 400;
  std::atomic<long> nb_ok{0};
  std::unique_ptr<vertex> gate(new_vertex([] {}));
  std::unique_ptr<vertex> end(new_vertex([] {}));
  std::vector<std::unique_ptr<vertex>> vs;
  for (long i = 0; i < n; i++) {
    vs.emplace_back(new_vertex([&, i] {
      volatile uint64_t pattern[64];
      for (int k = 0; k < 64; k++) {
        pattern[k] = rnd(seed, i, k);
      }
      wait(gate.get());
      bool same = true;
      for (int k = 0; k < 64; k++) {
        same = same && (pattern[k] == rnd(seed, i, k));
      }
      if (same) {
        nb_ok.fetch_add(1);
      }
    }));
    new_edge(vs.back().get(), end.get());
    release(vs.back().get());
  }
  release(end.get());
  // Let the vertices reach their wait before the gate opens.
  (void)fib_fork(16);
  release(gate.get());
  wait(end.get());
  return nb_ok.load() == n;
}

/*---------------------------------------------------------------------*/
/* Test 4: a vertex that runs many times */

// Claim: a vertex may be made ready again by its own notification, while the
// stack of its previous run is still in use, and that notification may fork.
//
// A ring of vertices passes a token around: each vertex, when it has run,
// re-arms its own count and counts the next one down, as the blocks of the
// stencil do. With a ring of one, a vertex makes itself ready from its own
// notification. Before it passes the token, a vertex forks, so that the
// notification may move to another worker. Each run checks that it is the
// only run in progress on its vertex, and that the token arrived in order.
struct ring_vertex : vertex {
  ring_vertex *next = nullptr;
  vertex *end = nullptr;
  std::atomic<long> *token = nullptr;
  std::atomic<bool> *ok = nullptr;
  long index = 0, ring_size = 0, laps_left = 0;
  std::atomic<int> running{0};

  void run() override {
    if (running.fetch_add(1) != 0) {
      ok->store(false);
    }
    long t = token->fetch_add(1);
    if (t % ring_size != index) {
      ok->store(false);
    }
    running.fetch_sub(1);
  }

  void notify() override {
    if ((index % 2 == 0) && (fib_fork(6) != 8)) {
      ok->store(false);
    }
    pending.store(1, std::memory_order_release); // re-arm before passing on
    bool last = (index + 1 == ring_size) && (--laps_left == 0);
    if (last) {
      count_down(end);
    } else {
      count_down(next);
    }
  }
};

static bool test_rerun(uint64_t seed) {
  bool ok_all = true;
  for (long ring_size : {1l, 2l, 3l, 17l}) {
    const long laps = 200 + rnd(seed, ring_size) % 100;
    std::atomic<long> token{0};
    std::atomic<bool> ok{true};
    std::unique_ptr<vertex> end(new_vertex([] {}));
    std::unique_ptr<ring_vertex[]> ring(new ring_vertex[ring_size]);
    for (long i = 0; i < ring_size; i++) {
      ring[i].next = &ring[(i + 1) % ring_size];
      ring[i].end = end.get();
      ring[i].token = &token;
      ring[i].ok = &ok;
      ring[i].index = i;
      ring[i].ring_size = ring_size;
      ring[i].laps_left = laps;
    }
    release(&ring[0]);
    wait(end.get());
    ok_all = ok_all && ok.load() && (token.load() == laps * ring_size);
  }
  return ok_all;
}

/*---------------------------------------------------------------------*/
/* Test 5: random DAGs, with an oracle */

// Claims: all of them at once. A random DAG is built while it runs, and an
// oracle checks the two properties that define a correct execution: every
// vertex runs exactly once, and only after every vertex with an edge to it
// has finished.
//
// Vertex i may depend only on vertices of smaller index, so the graph has no
// cycle. The vertices are released in a random order, each just after its
// in-edges are added, so that an edge's source may be unreleased, waiting,
// running, or finished when the edge is added. What a vertex does when it
// runs is random too: nothing, a fork-join computation, a wait for an
// earlier vertex, a group of vertices spawned on the fly (async-finish), or
// a vertex released just before a fork, which the fork then finds in its
// deque. Every vertex has an edge to one last vertex, which therefore gains
// in-edges while its predecessors run.
struct dag {
  uint64_t seed;
  long n;
  std::vector<vertex *> vs;
  std::vector<std::vector<long>> preds;
  std::unique_ptr<std::atomic<int>[]> nb_runs;
  std::unique_ptr<std::atomic<bool>[]> finished;
  std::atomic<long> nb_spawned{0}, nb_spawned_runs{0};
  std::atomic<bool> ok{true};
  std::atomic<made *> spawned{nullptr};
  vertex *last = nullptr;

  // A vertex made on the fly by a vertex that has an edge to last.
  void spawn(int depth) {
    nb_spawned.fetch_add(1);
    vertex *c = new_vertex([this, depth] {
      nb_spawned_runs.fetch_add(1);
      if (depth > 0) {
        spawn(depth - 1);
        spawn(depth - 1);
      }
    });
    remember(spawned, c);
    new_edge(c, last);
    release(c);
  }

  void body(long i) {
    if (nb_runs[i].fetch_add(1) != 0) {
      ok.store(false);
    }
    for (long p : preds[i]) {
      if (!finished[p].load()) {
        ok.store(false); // ran before a predecessor finished
      }
    }
    switch (rnd(seed, i, 100) % 6) {
    case 0:
      break;
    case 1:
      if (fib_fork(10) != 55) {
        ok.store(false);
      }
      break;
    case 2:
      if (i > 0) {
        long u = rnd(seed, i, 101) % i;
        wait(vs[u]);
        if (!finished[u].load()) {
          ok.store(false); // wait returned too early
        }
      }
      break;
    case 3:
      spawn(static_cast<int>(rnd(seed, i, 102) % 4));
      break;
    case 4:
      spawn(0);
      if (fib_fork(8) != 21) { // finds the spawned vertex in the deque
        ok.store(false);
      }
      break;
    case 5:
      fork2join(
          [&] {
            if (i > 0) {
              wait(vs[rnd(seed, i, 103) % i]);
            }
          },
          [&] { spawn(1); });
      break;
    }
    finished[i].store(true);
  }

  bool run() {
    vs.resize(n);
    preds.resize(n);
    nb_runs.reset(new std::atomic<int>[n]);
    finished.reset(new std::atomic<bool>[n]);
    last = new_vertex([] {});
    std::vector<long> order(n);
    for (long i = 0; i < n; i++) {
      nb_runs[i].store(0);
      finished[i].store(false);
      vs[i] = new_vertex([this, i] { body(i); });
      order[i] = i;
      if (i > 0) {
        // Mostly a few predecessors; now and then, many.
        long degree = rnd(seed, i, 1) % 4;
        if (rnd(seed, i, 2) % 40 == 0) {
          degree = std::min<long>(i, 50);
        }
        // Some vertices are popular, so that their out-sets grow long.
        for (long d = 0; d < degree; d++) {
          long p = (rnd(seed, i, 3 + d) % 5 == 0)
                       ? static_cast<long>(rnd(seed, d, 9) % std::min<long>(i, 4))
                       : static_cast<long>(rnd(seed, i, 50 + d) % i);
          preds[i].push_back(p);
        }
      }
    }
    for (long i = n - 1; i > 0; i--) {
      std::swap(order[i], order[rnd(seed, i, 7) % (i + 1)]);
    }
    // Add the in-edges of each vertex and release it, several at a time.
    auto start_one = [&](long k) {
      long i = order[k];
      for (long p : preds[i]) {
        new_edge(vs[p], vs[i]);
      }
      new_edge(vs[i], last);
      release(vs[i]);
    };
    parallel_for(0, n, start_one);
    release(last);
    wait(last);
    bool all = ok.load();
    for (long i = 0; i < n; i++) {
      all = all && (nb_runs[i].load() == 1) && finished[i].load();
      delete vs[i];
    }
    delete last;
    long nb_freed = free_all(spawned);
    return all && (nb_freed == nb_spawned.load()) &&
           (nb_spawned_runs.load() == nb_spawned.load());
  }
};

static bool test_random_dags(uint64_t seed) {
  bool ok = true;
  for (int i = 0; i < 6; i++) {
    dag d;
    d.seed = rnd(seed, i, 77);
    d.n = 50 + static_cast<long>(rnd(seed, i, 78) % 400);
    ok = ok && d.run();
  }
  return ok;
}

/*---------------------------------------------------------------------*/
/* Driver */

static std::atomic<long> progress{0};
static std::atomic<const char *> current{"startup"};

int main(int argc, char **argv) {
  int nb_rounds = (argc > 1) ? std::atoi(argv[1]) : 3;
  uint64_t seed = (argc > 2) ? std::strtoull(argv[2], nullptr, 10) : 1;
  const char *s = std::getenv("MTP_STRESS_TIMEOUT");
  long timeout_s = (s != nullptr) ? std::atol(s) : 120;

  // The watchdog: a test that makes no progress for timeout_s seconds has
  // lost a task or a stack.
  std::atomic<bool> stop{false};
  std::thread watchdog([&] {
    long seen = -1;
    long idle_s = 0;
    while (!stop.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      long p = progress.load();
      if (p != seen) {
        seen = p;
        idle_s = 0;
      } else if ((idle_s += 1) > timeout_s * 5) {
        std::printf("HANG in %s\n", current.load());
        std::fflush(stdout);
        std::_Exit(2);
      }
    }
  });

  struct {
    const char *name;
    bool (*run)(uint64_t);
  } tests[] = {{"waiting_forks", test_waiting_forks},
               {"wait_races", test_wait_races},
               {"many_stacks", test_many_stacks},
               {"rerun", test_rerun},
               {"random_dags", test_random_dags}};

  bool ok = true;
  for (int round = 0; round < nb_rounds; round++) {
    for (auto &t : tests) {
      bool r = false;
      current.store(t.name);
      launch([&] { r = t.run(rnd(seed, round)); });
      progress.fetch_add(1);
      if (!r) {
        std::printf("round %d: %s WRONG (seed %llu)\n", round, t.name,
                    static_cast<unsigned long long>(seed));
        ok = false;
      }
    }
  }
  stop.store(true);
  watchdog.join();
  std::printf("%s\n", ok ? "ok" : "FAILED");
  return ok ? 0 : 1;
}
