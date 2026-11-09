#pragma once

// microtaskparts: native fork join in a few hundred lines.
//
// A teaching companion to ../main.tex. It follows, step for step, the
// protocol TaskPaRTS uses to implement fork2join (fork2join in
// include/taskparts/taskparts_internal.hpp, native_fork_join_scheduler_family
// in src/taskparts.cpp), and leaves out everything else: elastic scheduling,
// instrumentation, CPU pinning, and the persistent worker pool.
//
// It is fast enough to measure the costs of the technique itself (Appendix D
// of ../main.tex), but it is not meant for benchmarking applications. Idle
// workers spin, yielding between steal attempts, instead of sleeping; each
// top-level call to fork2join starts and stops its own worker threads; and
// there is no instrumentation and no ParlayLib integration. For benchmarking
// applications, use TaskPaRTS.

#include "context.hpp"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <thread>
#include <vector>

namespace microtaskparts {

/*---------------------------------------------------------------------*/
/* Vertices */

// What the scheduler loop does with a vertex (TaskPaRTS: continuation_action).
enum class action {
  initialize, // never run: give it a fresh stack, then start it
  resume,     // suspended inside fork2join: restore its saved context
  finish      // when it next yields to the scheduler, it has finished
};

// A vertex of the fork-join DAG (TaskPaRTS: native_fork_join_vertex). As in
// TaskPaRTS, the in-counter, the out-set, and the context each start a cache
// line. With them packed together, we measured fib running about twice as
// slowly per worker once several workers ran (on an Apple M5), even though
// no two workers touch the same vertex in the common case.
struct vertex {
  alignas(64) std::atomic<int> incounter{0}; // unfinished predecessors
  alignas(64) vertex *outset = nullptr;      // the successor; null for the root
  action act = action::initialize;
  alignas(64) context ctx;
  char *stack = nullptr; // set only for vertices started on a fresh stack

  virtual void run() = 0;
  virtual ~vertex() { std::free(stack); }
};

template <typename F> struct thunk_vertex : vertex {
  F &body;
  explicit thunk_vertex(F &body) : body(body) {}
  void run() override { body(); }
};

/*---------------------------------------------------------------------*/
/* Work-stealing deque */

// The deque of Arora, Blumofe, and Plaxton (SPAA 1998), as in TaskPaRTS (abp
// in src/taskparts.cpp), plus one fence in steal. The owner pushes and pops
// at the bottom, and thieves take from the top. When the owner empties the
// deque, it resets both ends and bumps the tag, so that a thief holding the
// old age fails its compare-and-swap.
class alignas(64) abp {
  struct alignas(8) age_t {
    uint32_t tag;
    uint32_t top;
  };
  struct alignas(64) slot { // one per cache line, to avoid false sharing
    std::atomic<vertex *> v{nullptr};
  };
  // The deque holds at most one vertex per call to fork2join in progress on
  // the current task stack, far fewer than this.
  static constexpr uint32_t capacity = 10000;

  std::atomic<uint32_t> bot{0};
  std::atomic<age_t> age{age_t{0, 0}};
  slot slots[capacity];

public:
  void push(vertex *v) {
    uint32_t b = bot.load(std::memory_order_relaxed);
    if (b == capacity) {
      std::fprintf(stderr, "microtaskparts: deque overflow\n");
      std::abort();
    }
    slots[b].v.store(v, std::memory_order_release);
    bot.store(b + 1, std::memory_order_seq_cst);
  }

  vertex *pop() {
    uint32_t b = bot.load(std::memory_order_relaxed);
    if (b == 0) {
      return nullptr;
    }
    b--;
    bot.store(b, std::memory_order_release);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vertex *v = slots[b].v.load(std::memory_order_acquire);
    age_t old_age = age.load(std::memory_order_acquire);
    if (b > old_age.top) {
      return v; // no thief can reach slot b
    }
    // Slot b holds the last vertex, or was stolen. Reset the deque, and race
    // any thief for slot b.
    bot.store(0, std::memory_order_release);
    age_t new_age{old_age.tag + 1, 0};
    if ((b == old_age.top) && age.compare_exchange_strong(old_age, new_age)) {
      return v;
    }
    age.store(new_age, std::memory_order_seq_cst);
    return nullptr;
  }

  vertex *steal() {
    age_t old_age = age.load(std::memory_order_acquire);
    // Pairs with the fence in pop, as in the Chase-Lev deque of Le et al.
    // (PPoPP 2013): if the owner may have taken slot top without a
    // compare-and-swap, we see its decrement of bot.
    std::atomic_thread_fence(std::memory_order_seq_cst);
    uint32_t b = bot.load(std::memory_order_acquire);
    if (b <= old_age.top) {
      return nullptr;
    }
    vertex *v = slots[old_age.top].v.load(std::memory_order_acquire);
    age_t new_age{old_age.tag, old_age.top + 1};
    return age.compare_exchange_strong(old_age, new_age) ? v : nullptr;
  }
};

/*---------------------------------------------------------------------*/
/* Workers */

struct alignas(64) worker {
  // The scheduler loop runs on the worker thread's own stack. Its context is
  // saved here while a vertex runs, and restored when the vertex yields.
  context loop_ctx;
  vertex *current = nullptr;
  uint64_t nb_steal_attempts = 0; // seeds the choice of victim

  // Ready vertices: a private slot, plus a deque shared with thieves.
  // (TaskPaRTS: native_fork_join_deque_family, whose slot is called bottom.)
  vertex *next = nullptr;
  abp deque;
};

inline std::unique_ptr<worker[]> workers;
inline size_t nb_workers = 1;
inline std::atomic<bool> done{false};
inline bool in_scheduler = false;
inline thread_local size_t tls_id = 0;

// The id of the worker running the caller. A suspended computation may be
// resumed by a different thread, but compilers assume a function never
// changes threads, and so may reuse a thread-local address computed before
// the switch. Not inlining, plus the asm barrier, forces a fresh lookup. The
// barrier emits no instruction, but the lookup is not free (on macOS, it
// calls _tlv_get_addr), so callers look up the worker only when it may have
// changed, and pass it around otherwise.
__attribute__((noinline)) inline size_t get_my_id() {
  asm volatile("" ::: "memory");
  return tls_id;
}

inline size_t get_nb_workers() { return nb_workers; }

inline worker &me() { return workers[get_my_id()]; }

/*---------------------------------------------------------------------*/
/* Ready vertices */

// Make v ready on w, the worker running the caller. v takes the private
// slot, and the slot's previous occupant moves to the shared deque. So the
// vertex scheduled last can never be stolen, which fork2join relies on: it
// schedules a vertex before saving the vertex's context. The upshot is that
// only vertices that have never run reach the shared deque.
inline void schedule(worker &w, vertex *v) {
  if (w.next != nullptr) {
    w.deque.push(w.next);
  }
  w.next = v;
}

inline vertex *pop(worker &w) {
  if (vertex *v = w.next) {
    w.next = nullptr;
    return v;
  }
  return w.deque.pop();
}

// A 64-bit hash, as in TaskPaRTS (hash in src/taskparts.cpp).
inline uint64_t hash(uint64_t u) {
  uint64_t v = u * 3935559000370003845ul + 2691343689449507681ul;
  v ^= v >> 21;
  v ^= v << 37;
  v ^= v >> 4;
  v *= 4768777513237032717ul;
  v ^= v << 20;
  v ^= v >> 41;
  v ^= v << 5;
  return v;
}

// Try to steal from a worker other than w, chosen at random. As in TaskPaRTS
// (random_other_worker), the random number hashes the id of w and a count
// of its attempts. Only w touches that count, so it needs no
// synchronization.
inline vertex *steal(worker &w) {
  size_t my_id = &w - &workers[0];
  uint64_t r = hash(my_id) + hash(w.nb_steal_attempts++);
  size_t victim = r % (nb_workers - 1);
  victim += (victim >= my_id) ? 1 : 0;
  return workers[victim].deque.steal();
}

/*---------------------------------------------------------------------*/
/* Context switching */

// Save the current context in from, and switch to to. Returns once someone
// restores from.
inline void swap(context &from, context &to) { mtp_context_swap(&from, &to); }

// Switch to to, discarding the current context.
[[noreturn]] inline void throw_to(context &to) { mtp_context_restore(&to); }

static constexpr size_t stack_szb = 512 * 1024; // as in TaskPaRTS

// Every fresh stack starts here (TaskPaRTS: enter).
[[noreturn]] inline void vertex_entry() {
  me().current->run();
  // run() may return on a different worker than the one that called it.
  throw_to(me().loop_ctx);
}

// (TaskPaRTS: new_continuation and initialize_new_continuation.)
inline void new_context(vertex *v) {
  v->stack = static_cast<char *>(std::malloc(stack_szb));
  context_init(v->ctx, v->stack, stack_szb, &vertex_entry);
  v->act = action::finish;
}

/*---------------------------------------------------------------------*/
/* Scheduler loop */

// Tell v's successor that v has finished. The loop calls this only once
// control is off v's stack: v may live in the frame of its successor, and
// as soon as the counter reaches zero, the successor can resume on another
// worker, return, and pop that frame. So v is not touched after the
// decrement.
inline void notify(worker &w, vertex *v) {
  vertex *u = v->outset;
  if (u == nullptr) { // v is the root
    done.store(true);
    return;
  }
  if (u->incounter.fetch_sub(1, std::memory_order_acq_rel) == 1) {
    schedule(w, u);
  }
}

// The only place where vertices are started, resumed, and finished. It runs
// on the worker thread's own stack and never migrates, so w stays valid.
inline void worker_loop() {
  worker &w = me();
  while (!done.load()) {
    vertex *v = pop(w);
    if (v == nullptr) {
      if ((nb_workers > 1) && ((v = steal(w)) != nullptr)) {
        assert(v->act == action::initialize);
        schedule(w, v);
      } else {
        std::this_thread::yield();
      }
      continue;
    }
    assert(v->act != action::finish);
    w.current = v;
    if (v->act == action::initialize) {
      new_context(v);
    }
    swap(w.loop_ctx, v->ctx);
    // Back on our own stack: v has finished, or is suspended in fork2join.
    if (v->act == action::finish) {
      notify(w, v);
    }
  }
}

// Run f as the root vertex, on nb_workers threads; the caller is worker 0.
template <typename F> void launch(F &&f) {
  thunk_vertex root(f);
  const char *s = std::getenv("TASKPARTS_NUM_WORKERS");
  nb_workers = (s != nullptr)
                   ? std::max(1, std::atoi(s))
                   : std::max(1u, std::thread::hardware_concurrency());
  workers.reset(new worker[nb_workers]);
  done.store(false);
  in_scheduler = true;
  tls_id = 0;
  schedule(workers[0], &root);
  std::vector<std::thread> threads;
  for (size_t i = 1; i < nb_workers; i++) {
    threads.emplace_back([i] {
      tls_id = i;
      worker_loop();
    });
  }
  worker_loop();
  for (auto &t : threads) {
    t.join();
  }
  in_scheduler = false;
}

/*---------------------------------------------------------------------*/
/* Fork join */

// Run f1() and f2(), possibly in parallel, and return when both are done.
// The numbered steps match Figure 2 of ../main.tex.
template <typename F1, typename F2> void fork2join(F1 &&f1, F2 &&f2) {
  if (!in_scheduler) {
    launch([&] { fork2join(f1, f2); });
    return;
  }
  auto nop = [] {};
  thunk_vertex v1(nop); // the parent, while it runs f1
  thunk_vertex v2(f2);
  // The worker running us changes only across the calls to f1 and f2, and
  // across the second return from the save at (1); the swaps at (3), (5),
  // and (6) resume us on the same worker, from its private slot. So we look
  // up the worker once per stretch in between.
  worker *w = &me();
  vertex &v0 = *w->current; // the parent

  // (1) Save the parent's context. If v2 is stolen, the parent resumes here,
  // on whichever worker finishes last.
  if (mtp_context_save(&v0.ctx)) {
    v0.act = action::finish;
    return;
  }
  v0.act = action::resume;

  // (2) Fork: v0 waits for v1 and v2. v1 takes the private slot, pushing v2
  // into the shared deque, where thieves can find it.
  v1.outset = &v0;
  v2.outset = &v0;
  v0.incounter.store(2, std::memory_order_relaxed);
  schedule(*w, &v2);
  schedule(*w, &v1);

  // (3) Yield to the scheduler, which pops v1 and resumes us right here, now
  // running as v1. Then run f1 on this stack.
  v1.act = action::resume;
  swap(v1.ctx, w->loop_ctx);
  v1.act = action::finish;
  f1();
  w = &me();

  // (4) Try to take v2 back. f1 may have returned on another worker, but
  // only if v2 was stolen, in which case the pop fails there too.
  vertex *v = pop(*w);
  if (v == nullptr) {
    // v2 was stolen. Abandon this stack, leaving it suspended at (1), and
    // return to the scheduler, which notifies v0 that v1 has finished.
    // Whichever notification comes last makes v0 ready, and the scheduler
    // then resumes it at (1).
    throw_to(w->loop_ctx);
  }
  assert(v == &v2);

  // (5) v2 was not stolen. Yield to the scheduler, which notifies v0 that
  // v1 has finished, then resumes us here as v2. Run f2 on this same stack.
  v2.act = action::resume;
  schedule(*w, &v2);
  swap(v2.ctx, w->loop_ctx);
  v2.act = action::finish;
  f2();
  w = &me();

  // (6) Yield to the scheduler, which notifies v0 that v2 has finished,
  // making v0 ready, and resumes us here as v0.
  swap(v0.ctx, w->loop_ctx);
  v0.act = action::finish;
}

} // namespace microtaskparts
