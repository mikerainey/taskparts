#pragma once

// microtaskparts: native fork join in a few hundred lines.
//
// A teaching companion to ../main.tex. It implements the algorithm of that
// paper, step for step, and nothing else: no elastic scheduling, no
// instrumentation, no CPU pinning, and no persistent worker pool.
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
/* Joins and tasks */

// The join of one call to fork2join. It lives in the frame of that call, and
// is used only if the second branch is stolen.
struct join {
  // How many of the two branches have yet to reach the join.
  std::atomic<int> pending{2};
  // The parent, suspended at the join: restoring k makes fork2join return.
  context k;
};

// The second branch of a call to fork2join, which thieves may steal. It
// lives in the frame of that call too.
struct task {
  join *j;               // the join to report to; null for the root
  char *stack = nullptr; // the fresh stack it runs on, if it is stolen
  context k;             // where it starts on that stack

  explicit task(join *j) : j(j) {}
  virtual void run() = 0;
  virtual ~task() { std::free(stack); }
};

template <typename F> struct thunk_task : task {
  F &body;
  thunk_task(F &body, join *j) : task(j), body(body) {}
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
    std::atomic<task *> v{nullptr};
  };
  // The deque holds at most one task per call to fork2join in progress on
  // the current task stack, far fewer than this.
  static constexpr uint32_t capacity = 10000;

  std::atomic<uint32_t> bot{0};
  std::atomic<age_t> age{age_t{0, 0}};
  slot slots[capacity];

public:
  void push(task *v) {
    uint32_t b = bot.load(std::memory_order_relaxed);
    if (b == capacity) {
      std::fprintf(stderr, "microtaskparts: deque overflow\n");
      std::abort();
    }
    slots[b].v.store(v, std::memory_order_release);
    bot.store(b + 1, std::memory_order_seq_cst);
  }

  task *pop() {
    uint32_t b = bot.load(std::memory_order_relaxed);
    if (b == 0) {
      return nullptr;
    }
    b--;
    bot.store(b, std::memory_order_release);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    task *v = slots[b].v.load(std::memory_order_acquire);
    age_t old_age = age.load(std::memory_order_acquire);
    if (b > old_age.top) {
      return v; // no thief can reach slot b
    }
    // Slot b holds the last task, or was stolen. Reset the deque, and race
    // any thief for slot b.
    bot.store(0, std::memory_order_release);
    age_t new_age{old_age.tag + 1, 0};
    if ((b == old_age.top) && age.compare_exchange_strong(old_age, new_age)) {
      return v;
    }
    age.store(new_age, std::memory_order_seq_cst);
    return nullptr;
  }

  task *steal() {
    age_t old_age = age.load(std::memory_order_acquire);
    // Pairs with the fence in pop, as in the Chase-Lev deque of Le et al.
    // (PPoPP 2013): if the owner may have taken slot top without a
    // compare-and-swap, we see its decrement of bot.
    std::atomic_thread_fence(std::memory_order_seq_cst);
    uint32_t b = bot.load(std::memory_order_acquire);
    if (b <= old_age.top) {
      return nullptr;
    }
    task *v = slots[old_age.top].v.load(std::memory_order_acquire);
    age_t new_age{old_age.tag, old_age.top + 1};
    return age.compare_exchange_strong(old_age, new_age) ? v : nullptr;
  }
};

/*---------------------------------------------------------------------*/
/* Workers */

struct alignas(64) worker {
  // The scheduler loop runs on the worker thread's own stack. Its context is
  // saved here while the worker runs client code on a task stack.
  context loop;
  // A message to the loop from the code that switches to it: the task to
  // start, on the way out, and the join that a branch has just reached, on
  // the way back.
  task *starting = nullptr;
  join *reached = nullptr;
  uint64_t nb_steal_attempts = 0; // seeds the choice of victim
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
// changed.
__attribute__((noinline)) inline size_t get_my_id() {
  asm volatile("" ::: "memory");
  return tls_id;
}

inline size_t get_nb_workers() { return nb_workers; }

inline worker &me() { return workers[get_my_id()]; }

// A 64-bit hash.
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

// Try to steal from a worker other than w, chosen at random: the random
// number hashes the id of w and a count of its attempts. Only w touches
// that count, so it needs no synchronization.
inline task *steal(worker &w) {
  size_t my_id = &w - &workers[0];
  uint64_t r = hash(my_id) + hash(w.nb_steal_attempts++);
  size_t victim = r % (nb_workers - 1);
  victim += (victim >= my_id) ? 1 : 0;
  return workers[victim].deque.steal();
}

/*---------------------------------------------------------------------*/
/* Scheduler loop */

static constexpr size_t stack_szb = 512 * 1024;

// A branch running on worker w has reached join j (null for the root). Leave
// the current stack for good and tell the loop, which counts the branch only
// once we are off the stack.
[[noreturn]] inline void finish(worker &w, join *j) {
  if (j == nullptr) {
    done.store(true);
  }
  w.reached = j;
  mtp_context_restore(&w.loop);
}

// Every fresh stack starts here.
[[noreturn]] inline void task_entry() {
  task *t = me().starting;
  t->run();
  // run() may return on a different worker than the one that called it.
  finish(me(), t->j);
}

// Runs on the worker thread's own stack and never migrates, so w stays
// valid. first, if not null, is a task to start with.
//
// The loop is the only place where a join is counted down. A branch that
// reaches a join first switches here, so by the time another worker can see
// the count reach zero and resume the parent, the branch is off its stack:
// the parent's own stack, for the first branch, and a stack that the parent
// frees when fork2join returns, for the second.
inline void worker_loop(task *first) {
  worker &w = me();
  task *t = first;
  while (!done.load()) {
    context *next = nullptr;
    if (join *j = w.reached) {
      w.reached = nullptr;
      if (j->pending.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        next = &j->k; // both branches are done: resume the parent
      }
    } else if ((t != nullptr) ||
               ((nb_workers > 1) && ((t = steal(w)) != nullptr))) {
      t->stack = static_cast<char *>(std::malloc(stack_szb));
      context_init(t->k, t->stack, stack_szb, &task_entry);
      w.starting = t;
      next = &t->k;
      t = nullptr;
    } else {
      std::this_thread::yield();
    }
    if (next != nullptr) {
      mtp_context_swap(&w.loop, next);
      // Back on our own stack: a branch has reached its join.
    }
  }
}

// Run f as the root task, on nb_workers threads; the caller is worker 0.
// Kept out of line: it runs once, and inlined into a caller of fork2join it would
// make every call of that caller save the registers it uses.
template <typename F> __attribute__((noinline)) void launch(F &&f) {
  thunk_task root(f, nullptr);
  const char *s = std::getenv("TASKPARTS_NUM_WORKERS");
  nb_workers = (s != nullptr)
                   ? std::max(1, std::atoi(s))
                   : std::max(1u, std::thread::hardware_concurrency());
  workers.reset(new worker[nb_workers]);
  done.store(false);
  in_scheduler = true;
  tls_id = 0;
  std::vector<std::thread> threads;
  for (size_t i = 1; i < nb_workers; i++) {
    threads.emplace_back([i] {
      tls_id = i;
      worker_loop(nullptr);
    });
  }
  worker_loop(&root);
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
  join j;
  thunk_task t(f2, &j);

  // (1) Fork: make the second branch stealable, and run the first.
  me().deque.push(&t);
  f1();

  // (2) Try to take the second branch back. f1 may have returned on another
  // worker, so look the worker up again. But f1 moves only if a task forked
  // inside it was stolen, and thieves take the oldest task first, so then t
  // was stolen too, and the pop fails on that worker as well.
  worker &w = me();
  if (task *u = w.deque.pop()) {
    // (3) Not stolen: run the second branch here, as an ordinary call.
    assert(u == &t);
    (void)u;
    f2();
    return;
  }

  // (4) Stolen: suspend this stack at the join, and switch to the scheduler
  // loop, which counts this branch. Whichever branch is counted last resumes
  // us here, on its worker.
  w.reached = &j;
  mtp_context_swap(&j.k, &w.loop);
}

} // namespace microtaskparts
