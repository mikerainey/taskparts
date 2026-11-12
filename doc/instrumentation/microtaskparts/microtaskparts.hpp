#pragma once

// microtaskparts: native fork join, with instrumentation.
//
// A teaching companion to ../main.tex. It is the micro taskparts of
// ../../native-fork-join, plus the instrumentation that ../main.tex
// describes, and nothing else: no elastic scheduling, no CPU pinning, and no
// persistent worker pool. Idle workers spin, yielding between steal
// attempts, instead of sleeping, and each top-level call to fork2join starts
// and stops its own worker threads.
//
// The instrumentation is off unless a build turns it on:
//
//   -DMTP_STATS        time each worker spends working, scheduling and
//                      idle, and count steals (see stats, below)
//   -DMTP_LOG          MTP_STATS, plus a log of every change of state and
//                      every steal, which write_trace writes for Perfetto
//   -DMTP_COUNT_FORKS  MTP_STATS, plus a count of forks, the one item that
//                      costs something on a fork that is not stolen
//
// One more flag, -DMTP_SERIAL, makes fork2join two ordinary calls. That is
// the sequential program, whose running time is the baseline of a speedup
// (Appendix A of ../main.tex).

#include "context.hpp"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <chrono>
#include <thread>
#include <vector>

#if defined(MTP_LOG) || defined(MTP_COUNT_FORKS)
#if !defined(MTP_STATS)
#define MTP_STATS
#endif
#endif

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
/* Instrumentation: what each worker records */

// At any time, a worker is in one of these states.
enum state : uint32_t {
  working = 0, // running client code, on a task stack
  scheduling,  // in its scheduler loop, with something to do
  idle,        // in its scheduler loop, failing to steal
  stopped,
  nb_states
};

static constexpr const char *state_names[nb_states] = {"work", "sched",
                                                       "idle", "stopped"};

// The processor's cycle counter, or the nearest thing to it: one
// instruction, and no call. launch measures how long a tick lasts.
inline uint64_t ticks() {
#if defined(__aarch64__)
  uint64_t t;
  asm volatile("mrs %0, cntvct_el0" : "=r"(t));
  return t;
#else
  return __builtin_ia32_rdtsc();
#endif
}

// A log entry: at time ticks, the worker entered a state, or stole a task
// from the worker numbered arg.
struct event {
  static constexpr uint32_t steal = nb_states;
  uint64_t ticks;
  uint32_t what;
  uint32_t arg;
};

// What one worker records. Only that worker writes it, and nobody else
// reads it until the workers have stopped, so none of it is atomic.
struct recorder {
  state current = scheduling;
  uint64_t since = 0;              // when the worker entered current
  uint64_t ticks_in[nb_states] = {}; // total time in each state so far
  uint64_t nb_steals = 0;
  uint64_t nb_forks = 0;
  std::vector<event> log;
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
#if defined(MTP_STATS)
  // After the deque, so that the fields above sit where they do in a build
  // with no instrumentation, and fork2join compiles to the same code. The
  // deque ends at a cache-line boundary, and so does the worker, so the
  // recorder shares no line with a deque, which thieves write to, nor with
  // another worker's recorder.
  recorder rec;
#endif
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
  task *t = workers[victim].deque.steal();
#if defined(MTP_STATS)
  if (t != nullptr) {
    w.rec.nb_steals++;
#if defined(MTP_LOG)
    w.rec.log.push_back(event{ticks(), event::steal, (uint32_t)victim});
#endif
  }
#endif
  return t;
}

// Record that worker w is now in state s. This is the only place where a
// worker reads the clock, and only the scheduler loop calls it: w is the
// worker whose loop is running, and client code never comes here.
inline void mark([[maybe_unused]] worker &w, [[maybe_unused]] state s) {
#if defined(MTP_STATS)
  recorder &r = w.rec;
  if (r.current == s) {
    return; // e.g. still idle, after one more failed steal
  }
  uint64_t now = ticks();
  r.ticks_in[r.current] += now - r.since;
  r.current = s;
  r.since = now;
#if defined(MTP_LOG)
  r.log.push_back(event{now, s, 0});
#endif
#endif
}

// The hook that fork2join runs on each fork, on the worker w that makes the
// fork. A program may define its own before including this file.
#if !defined(MTP_ON_FORK)
#if defined(MTP_COUNT_FORKS)
#define MTP_ON_FORK(w) ((w).rec.nb_forks++)
#else
#define MTP_ON_FORK(w) ((void)(w))
#endif
#endif

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
      mark(w, scheduling);
      t->stack = static_cast<char *>(std::malloc(stack_szb));
      context_init(t->k, t->stack, stack_szb, &task_entry);
      w.starting = t;
      next = &t->k;
      t = nullptr;
    } else {
      mark(w, idle);
      std::this_thread::yield();
    }
    if (next != nullptr) {
      mark(w, working);
      mtp_context_swap(&w.loop, next);
      // Back on our own stack: a branch has reached its join.
      mark(w, scheduling);
    }
  }
  mark(w, stopped);
}

/*---------------------------------------------------------------------*/
/* Instrumentation: what a launch reports */

// The totals of the most recent launch, with times in seconds. The three
// times are summed over the workers, and together they account for
// nb_workers * exectime, less the moment between a worker's stopping and
// the end of the launch. The counts are zero unless the build records them.
struct stats {
  size_t nb_workers = 0;
  double exectime = 0.0; // wall-clock time of the launch
  double work = 0.0;
  double sched = 0.0;
  double idle = 0.0;
  uint64_t nb_steals = 0;
  uint64_t nb_forks = 0;
};

inline stats last_stats;
inline uint64_t launch_ticks = 0;     // when the most recent launch began
inline double seconds_per_tick = 0.0; // as measured over that launch

inline void print_stats(FILE *f) {
  const stats &s = last_stats;
  std::fprintf(f,
               "{\"nb_workers\": %zu, \"exectime\": %.6f, \"work\": %.6f, "
               "\"sched\": %.6f, \"idle\": %.6f, \"nb_steals\": %llu, "
               "\"nb_forks\": %llu}\n",
               s.nb_workers, s.exectime, s.work, s.sched, s.idle,
               (unsigned long long)s.nb_steals, (unsigned long long)s.nb_forks);
}

#if defined(MTP_STATS)
inline void collect_stats(double exectime, uint64_t end_ticks) {
  seconds_per_tick = exectime / (double)(end_ticks - launch_ticks);
  stats s;
  s.nb_workers = nb_workers;
  s.exectime = exectime;
  uint64_t in[nb_states] = {};
  for (size_t i = 0; i < nb_workers; i++) {
    const recorder &r = workers[i].rec;
    for (int k = 0; k < nb_states; k++) {
      in[k] += r.ticks_in[k];
    }
    s.nb_steals += r.nb_steals;
    s.nb_forks += r.nb_forks;
  }
  s.work = (double)in[working] * seconds_per_tick;
  s.sched = (double)in[scheduling] * seconds_per_tick;
  s.idle = (double)in[idle] * seconds_per_tick;
  last_stats = s;
}
#endif

#if defined(MTP_LOG)
// Write the log of the most recent launch in the JSON trace format that
// Perfetto (ui.perfetto.dev) reads: one track per worker, one slice per
// stay in a state, and one instant per steal. Times are in microseconds.
inline void write_trace(const char *path) {
  FILE *f = std::fopen(path, "w");
  if (f == nullptr) {
    std::perror(path);
    return;
  }
  auto us = [](uint64_t t) {
    return (double)(t - launch_ticks) * seconds_per_tick * 1e6;
  };
  std::fprintf(f, "{\"displayTimeUnit\": \"ns\", \"traceEvents\": [\n");
  for (size_t i = 0; i < nb_workers; i++) {
    std::fprintf(f,
                 "%s{\"name\": \"thread_name\", \"ph\": \"M\", \"pid\": 0, "
                 "\"tid\": %zu, \"args\": {\"name\": \"worker %zu\"}}",
                 (i == 0) ? "" : ",\n", i, i);
    const event *open = nullptr; // the state the worker is in
    for (const event &e : workers[i].rec.log) {
      if (e.what == event::steal) {
        std::fprintf(f,
                     ",\n{\"name\": \"steal\", \"ph\": \"i\", \"s\": \"t\", "
                     "\"pid\": 0, \"tid\": %zu, \"ts\": %.3f, "
                     "\"args\": {\"victim\": %u}}",
                     i, us(e.ticks), e.arg);
        continue;
      }
      if (open != nullptr) {
        std::fprintf(f,
                     ",\n{\"name\": \"%s\", \"ph\": \"X\", \"pid\": 0, "
                     "\"tid\": %zu, \"ts\": %.3f, \"dur\": %.3f}",
                     state_names[open->what], i, us(open->ticks),
                     us(e.ticks) - us(open->ticks));
      }
      open = &e;
    }
  }
  std::fprintf(f, "\n]}\n");
  std::fclose(f);
}
#endif

// Run f as the root task, on nb_workers threads; the caller is worker 0.
//
// Not inlined, because fork2join is: it becomes part of each client function
// that forks, and whatever launch needs, such as the floating-point
// registers that its instrumentation uses, that function would save and
// restore on every call.
template <typename F> __attribute__((noinline)) void launch(F &&f) {
  thunk_task root(f, nullptr);
  const char *s = std::getenv("TASKPARTS_NUM_WORKERS");
  nb_workers = (s != nullptr)
                   ? std::max(1, std::atoi(s))
                   : std::max(1u, std::thread::hardware_concurrency());
#if defined(MTP_STATS)
  auto start = std::chrono::steady_clock::now();
  launch_ticks = ticks();
#endif
  workers.reset(new worker[nb_workers]);
#if defined(MTP_STATS)
  // Every worker starts out scheduling, at the start of the launch, so the
  // time it takes to start its thread counts as scheduling.
  for (size_t i = 0; i < nb_workers; i++) {
    workers[i].rec.since = launch_ticks;
#if defined(MTP_LOG)
    workers[i].rec.log.reserve(1 << 12);
    workers[i].rec.log.push_back(event{launch_ticks, scheduling, 0});
#endif
  }
#endif
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
#if defined(MTP_STATS)
  uint64_t end_ticks = ticks();
  std::chrono::duration<double> elapsed =
      std::chrono::steady_clock::now() - start;
  collect_stats(elapsed.count(), end_ticks);
#endif
  in_scheduler = false;
}

/*---------------------------------------------------------------------*/
/* Fork join */

// Run f1() and f2(), possibly in parallel, and return when both are done.
// The numbered steps match Figure 2 of ../../native-fork-join/main.tex.
template <typename F1, typename F2> void fork2join(F1 &&f1, F2 &&f2) {
#if defined(MTP_SERIAL)
  // The sequential program: two calls, and no scheduler.
  f1();
  f2();
  return;
#endif
  if (!in_scheduler) {
    launch([&] { fork2join(f1, f2); });
    return;
  }
  join j;
  thunk_task t(f2, &j);

  // (1) Fork: make the second branch stealable, and run the first.
  worker &w1 = me();
  MTP_ON_FORK(w1);
  w1.deque.push(&t);
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
