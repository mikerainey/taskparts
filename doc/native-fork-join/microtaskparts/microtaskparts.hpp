#pragma once

// microtaskparts: native fork join in a few hundred lines.
//
// A teaching companion to ../main.tex. It follows, step for step, the
// protocol TaskPaRTS uses to implement fork2join (fork2join in
// include/taskparts/taskparts_internal.hpp, native_fork_join_scheduler_family
// in src/taskparts.cpp), and leaves out everything else: lock-free deques,
// elastic scheduling, instrumentation, CPU pinning, and the persistent worker
// pool. Use TaskPaRTS itself for performance measurements.

#include "context.hpp"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdlib>
#include <deque>
#include <memory>
#include <mutex>
#include <random>
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

// A vertex of the fork-join DAG (TaskPaRTS: native_fork_join_vertex).
struct vertex {
  std::atomic<int> incounter{0}; // number of unfinished predecessors
  vertex *outset = nullptr;      // the successor; null for the root
  action act = action::initialize;
  context ctx;
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
/* Workers */

struct alignas(64) worker {
  // The scheduler loop runs on the worker thread's own stack. Its context is
  // saved here while a vertex runs, and restored when the vertex yields.
  context loop_ctx;
  vertex *current = nullptr;

  // Ready vertices: a private slot, plus a deque shared with thieves.
  // (TaskPaRTS: native_fork_join_deque_family, whose slot is called bottom.)
  vertex *next = nullptr;
  std::mutex mutex;
  std::deque<vertex *> deque;

  std::minstd_rand rng;
};

inline std::unique_ptr<worker[]> workers;
inline size_t nb_workers = 1;
inline std::atomic<bool> done{false};
inline bool in_scheduler = false;
inline thread_local size_t tls_id = 0;

// The id of the worker running the caller. A suspended computation may be
// resumed by a different thread, but compilers assume a function never
// changes threads, and so may reuse a thread-local address computed before
// the switch. Not inlining, plus the asm barrier, forces a fresh lookup.
__attribute__((noinline)) inline size_t get_my_id() {
  asm volatile("" ::: "memory");
  return tls_id;
}

inline size_t get_nb_workers() { return nb_workers; }

inline worker &me() { return workers[get_my_id()]; }

/*---------------------------------------------------------------------*/
/* Ready vertices */

// Make v ready. v takes the private slot, and the slot's previous occupant
// moves to the shared deque. So the vertex scheduled last can never be
// stolen, which fork2join relies on: it schedules a vertex before saving the
// vertex's context. The upshot is that only vertices that have never run
// reach the shared deque.
inline void schedule(vertex *v) {
  worker &w = me();
  if (w.next != nullptr) {
    std::lock_guard<std::mutex> guard(w.mutex);
    w.deque.push_back(w.next);
  }
  w.next = v;
}

inline vertex *pop() {
  worker &w = me();
  if (vertex *v = w.next) {
    w.next = nullptr;
    return v;
  }
  std::lock_guard<std::mutex> guard(w.mutex);
  if (w.deque.empty()) {
    return nullptr;
  }
  vertex *v = w.deque.back();
  w.deque.pop_back();
  return v;
}

inline vertex *steal() {
  size_t my_id = get_my_id();
  size_t victim = workers[my_id].rng() % (nb_workers - 1);
  victim += (victim >= my_id) ? 1 : 0;
  worker &w = workers[victim];
  std::lock_guard<std::mutex> guard(w.mutex);
  if (w.deque.empty()) {
    return nullptr;
  }
  vertex *v = w.deque.front();
  w.deque.pop_front();
  return v;
}

/*---------------------------------------------------------------------*/
/* Context switching */

// Save the current context in from, and switch to to. Returns once someone
// restores from.
inline void swap(context &from, context &to) {
  if (mtp_context_save(&from) == 0) {
    mtp_context_restore(&to);
  }
}

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
inline void notify(vertex *v) {
  vertex *u = v->outset;
  if (u == nullptr) { // v is the root
    done.store(true);
    return;
  }
  if (u->incounter.fetch_sub(1, std::memory_order_acq_rel) == 1) {
    schedule(u);
  }
}

// The only place where vertices are started, resumed, and finished. It runs
// on the worker thread's own stack and never migrates.
inline void worker_loop() {
  worker &w = me();
  while (!done.load()) {
    vertex *v = pop();
    if (v == nullptr) {
      if ((nb_workers > 1) && ((v = steal()) != nullptr)) {
        assert(v->act == action::initialize);
        schedule(v);
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
      notify(v);
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
  for (size_t i = 0; i < nb_workers; i++) {
    workers[i].rng.seed(i + 1);
  }
  done.store(false);
  in_scheduler = true;
  tls_id = 0;
  schedule(&root);
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
  vertex &v0 = *me().current; // the parent

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
  schedule(&v2);
  schedule(&v1);

  // (3) Yield to the scheduler, which pops v1 and resumes us right here, now
  // running as v1. Then run f1 on this stack.
  v1.act = action::resume;
  swap(v1.ctx, me().loop_ctx);
  v1.act = action::finish;
  f1();

  // (4) Try to take v2 back. f1 may have returned on another worker, but
  // only if v2 was stolen, in which case the pop fails there too.
  vertex *v = pop();
  if (v == nullptr) {
    // v2 was stolen. Abandon this stack, leaving it suspended at (1), and
    // return to the scheduler, which notifies v0 that v1 has finished.
    // Whichever notification comes last makes v0 ready, and the scheduler
    // then resumes it at (1).
    throw_to(me().loop_ctx);
  }
  assert(v == &v2);

  // (5) v2 was not stolen. Yield to the scheduler, which notifies v0 that
  // v1 has finished, then resumes us here as v2. Run f2 on this same stack.
  v2.act = action::resume;
  schedule(&v2);
  swap(v2.ctx, me().loop_ctx);
  v2.act = action::finish;
  f2();

  // (6) Yield to the scheduler, which notifies v0 that v2 has finished,
  // making v0 ready, and resumes us here as v0.
  swap(v0.ctx, me().loop_ctx);
  v0.act = action::finish;
}

} // namespace microtaskparts
