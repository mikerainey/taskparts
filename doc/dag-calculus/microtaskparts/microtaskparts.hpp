#pragma once

// microtaskparts: native fork join and DAG calculus on one scheduler.
//
// A teaching companion to ../main.tex. It implements the algorithm of that
// paper, step for step, and nothing else: no elastic scheduling, no CPU
// pinning, and no persistent worker pool.
//
// It starts from the micro taskparts of the native fork join paper
// (../../native-fork-join/microtaskparts), and adds vertices: tasks that run
// once a count of unfinished predecessors reaches zero.
//
// Idle workers spin, yielding between steal attempts, instead of sleeping,
// and each call to launch starts and stops its own worker threads.
//
// Instrumentation is off unless a build turns it on, and then it adds lines
// to the scheduler without changing what the scheduler does:
//
//   -DMTP_STATS        time each worker spends working, scheduling and
//                      idle, and count steals and resumed stacks
//   -DMTP_LOG          MTP_STATS, plus a log of every change of state and
//                      every steal, which write_trace writes for Perfetto
//   -DMTP_COUNT_FORKS  MTP_STATS, plus a count of forks
//   -DMTP_COUNT_DAG    MTP_STATS, plus counts of vertices and of the
//                      operations of DAG calculus: edges, waits, and what
//                      became of them
//
// What is measured is one call to launch, from before its threads start
// until they have stopped (launch_stats, write_trace). A worker stays
// recorded as working while it runs the tasks of its own deque one after
// another, and reads the clock only when that deque is empty.
// -DMTP_MARK_EVERY_SWITCH makes it record every return to its loop as
// scheduling instead, at two readings of the clock per task.
//
// For testing, three things can be switched on, and cost nothing otherwise:
// - built with -fsanitize=address or -fsanitize=thread, the scheduler tells
//   the sanitizer about every switch between stacks;
// - with -DMTP_PERTURB, workers pause at random at the points where the
//   scheduler's races are decided;
// - with -DMTP_CHECK_STACKS, each stack records whether a task is using it,
//   and is overwritten when it is given back;
// - with -DMTP_CHECK_ORPHANS, the program stops if a computation ends while a
//   vertex is still ready or running: a vertex that nothing waited for
//   (Section "Continuations" of ../main.tex).

#include "context.hpp"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <chrono>
#include <cstdlib>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include <sys/mman.h>
#include <unistd.h>

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define MTP_ASAN 1
#endif
#if __has_feature(thread_sanitizer)
#define MTP_TSAN 1
#endif
#endif
#if defined(__SANITIZE_ADDRESS__) && !defined(MTP_ASAN)
#define MTP_ASAN 1
#endif
#if defined(__SANITIZE_THREAD__) && !defined(MTP_TSAN)
#define MTP_TSAN 1
#endif
#if defined(MTP_ASAN) || defined(MTP_TSAN)
#define MTP_FIBERS 1
#endif
#ifdef MTP_ASAN
#include <sanitizer/asan_interface.h>
#include <sanitizer/common_interface_defs.h>
#endif
#ifdef MTP_TSAN
#include <sanitizer/tsan_interface.h>
#endif
#if defined(MTP_LOG) || defined(MTP_COUNT_FORKS) || defined(MTP_COUNT_DAG) || \
    defined(MTP_MARK_EVERY_SWITCH)
#if !defined(MTP_STATS)
#define MTP_STATS
#endif
#endif
#ifdef MTP_CHECK_STACKS
#include <cstring>
#endif

namespace microtaskparts {

/*---------------------------------------------------------------------*/
/* Testing aids */

// What a sanitizer needs to know about a stack in order to follow a switch
// to it: its bounds, for the address sanitizer, and the thread sanitizer's
// own handle for it. Empty, and unused, in an ordinary build.
struct fiber {
#ifdef MTP_FIBERS
  const void *bottom = nullptr;
  size_t szb = 0;
  void *tsan = nullptr;
#endif
};

// Call just before switching from the current stack to the stack `to`. save
// is where to keep the address sanitizer's record of the current stack until
// it is resumed, or null if the current stack is being left for good.
//
// Always inlined: the thread sanitizer keeps a record of the calls in
// progress on each stack, and a call that began on one stack and returned on
// another would corrupt two of them.
__attribute__((always_inline)) inline void fiber_leave(void **save, const fiber &to) {
#ifdef MTP_ASAN
  __sanitizer_start_switch_fiber(save, to.bottom, to.szb);
#endif
#ifdef MTP_TSAN
  __tsan_switch_to_fiber(to.tsan, 0);
#endif
  (void)save;
  (void)to;
}

// Call just after arriving on a stack. save is what fiber_leave kept when
// this stack was left, or null on a fresh stack. If from is not null, it
// receives the bounds of the stack we came from.
__attribute__((always_inline)) inline void fiber_arrive(void *save, fiber *from = nullptr) {
#ifdef MTP_ASAN
  const void *bottom;
  size_t szb;
  __sanitizer_finish_switch_fiber(save, &bottom, &szb);
  if (from != nullptr) {
    from->bottom = bottom;
    from->szb = szb;
  }
#endif
  (void)save;
  (void)from;
}

// Pause at random. Called where the scheduler's races are decided, so that a
// test visits orders of events that are rare otherwise. Not inlined, for the
// reason given at get_my_id: the caller may have changed threads.
#ifdef MTP_PERTURB
__attribute__((noinline)) inline void perturb() {
  asm volatile("" ::: "memory");
  static thread_local uint64_t x =
      0x9e3779b97f4a7c15ull ^
      std::hash<std::thread::id>()(std::this_thread::get_id());
  x ^= x << 13;
  x ^= x >> 7;
  x ^= x << 17;
  if ((x & 3) == 0) {
    std::this_thread::yield();
  } else if ((x & 127) == 1) {
    std::this_thread::sleep_for(std::chrono::microseconds((x >> 8) & 63));
  }
}
#else
inline void perturb() {}
#endif

/*---------------------------------------------------------------------*/
/* Stacks of tasks */

#ifndef MTP_STACK_SZB
#define MTP_STACK_SZB (512 * 1024)
#endif
static constexpr size_t stack_szb = MTP_STACK_SZB;

inline size_t page_szb() {
  static const size_t szb = static_cast<size_t>(sysconf(_SC_PAGESIZE));
  return szb;
}

// A fresh stack of stack_szb bytes. The page below it is protected, so that a
// task that overflows its stack crashes instead of overwriting memory. Mapping
// and unmapping a stack are system calls that slow down every thread of the
// process, so a stack that a task has left is kept for reuse: by the worker
// that it was left on, and beyond a few, in a pool that the workers share
// (take_stack and put_stack, below).
#if defined(MTP_STATS)
// How many stacks have been mapped since the program started. None is
// unmapped before the workers stop, so within one call to launch it bounds
// the number of stacks that were in use at once.
inline std::atomic<uint64_t> nb_stacks_mapped{0};
#endif

inline char *new_stack() {
#if defined(MTP_STATS)
  nb_stacks_mapped.fetch_add(1, std::memory_order_relaxed);
#endif
  void *p = mmap(nullptr, page_szb() + stack_szb, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANON, -1, 0);
  if ((p == MAP_FAILED) || (mprotect(p, page_szb(), PROT_NONE) != 0)) {
    std::perror("microtaskparts: cannot allocate a stack");
    std::abort();
  }
  return static_cast<char *>(p) + page_szb();
}

inline void delete_stack(char *stack) {
  if (stack != nullptr) {
    munmap(stack - page_szb(), page_szb() + stack_szb);
  }
}

// The stacks that no worker keeps. All workers share them, so that no more
// stacks are mapped than were ever in use at once, plus the few that each
// worker keeps. A lock suffices: a worker comes here only when its own stacks
// run out or pile up.
inline std::mutex stack_pool_lock;
inline std::vector<char *> stack_pool;

__attribute__((noinline)) inline char *take_stack() {
  {
    std::lock_guard<std::mutex> guard(stack_pool_lock);
    if (!stack_pool.empty()) {
      char *stack = stack_pool.back();
      stack_pool.pop_back();
      return stack;
    }
  }
  return new_stack();
}

__attribute__((noinline)) inline void put_stack(char *stack) {
  std::lock_guard<std::mutex> guard(stack_pool_lock);
  stack_pool.push_back(stack);
}

// With MTP_CHECK_STACKS, the lowest word of a stack says whether a task is
// using it. A task's frames grow down from the other end, and never reach
// that word unless the stack is about to overflow.
inline void stack_taken(char *stack) {
#ifdef MTP_CHECK_STACKS
  auto in_use = reinterpret_cast<std::atomic<uint64_t> *>(stack);
  if (in_use->exchange(1) != 0) {
    std::fprintf(stderr, "microtaskparts: a stack was given to two tasks\n");
    std::abort();
  }
#endif
  (void)stack;
}

inline void stack_given_back(char *stack) {
#ifdef MTP_CHECK_STACKS
  auto in_use = reinterpret_cast<std::atomic<uint64_t> *>(stack);
  if (in_use->exchange(0) != 1) {
    std::fprintf(stderr, "microtaskparts: a stack was given back twice\n");
    std::abort();
  }
  // Anything that still runs on this stack, or reads it, now misbehaves.
  std::memset(stack + sizeof(uint64_t), 0xdd, stack_szb - sizeof(uint64_t));
#endif
#ifdef MTP_ASAN
  // The frames that were abandoned on this stack left it marked.
  __asan_unpoison_memory_region(stack, stack_szb);
#endif
  (void)stack;
}

/*---------------------------------------------------------------------*/
/* Joins, tasks, and vertices */

struct worker;

// The join of one call to fork2join. It lives in the frame of that call, and
// is used only if the second branch is stolen.
struct join {
  // How many of the two branches have yet to reach the join.
  std::atomic<int> pending{2};
  // The parent, suspended at the join: restoring k makes fork2join return.
  context k;
#ifdef MTP_FIBERS
  fiber fb; // the parent's stack
#endif
};

// Something that is ready to run and is not running. A worker's deque holds
// tasks, and whichever worker takes a task out of a deque starts it, once.
struct task {
  virtual void run() = 0;
  // Called on the task's stack after run() returns, just before the stack is
  // left for good: do what finishing this task requires. It may return on a
  // different worker than the one that called it.
  virtual void leave() = 0;
  // The context to resume, for a task that is a suspended stack; null for a
  // task that starts on a fresh stack.
  virtual context *saved() { return nullptr; }
#if defined(MTP_STATS)
  virtual bool is_vertex() const { return false; }
#endif
  virtual ~task() {}
};

// The second branch of a call to fork2join, which thieves may steal. It
// lives in the frame of that call too.
template <typename F> struct fork_task : task {
  F &body;
  join *j; // the join to report to
  fork_task(F &body, join *j) : body(body), j(j) {}
  void run() override { body(); }
  void leave() override;
};

// A vertex of a DAG: a task that becomes ready when its count of unfinished
// predecessors reaches zero. The count starts at one, for the release that
// the creator of the vertex owes it.
//
// The scheduler never frees a vertex, and does not touch one after its
// notify() has made its last call to count_down. So whoever waits for the
// vertices downstream of v may free v.
struct vertex : task {
  struct edge {
    vertex *target;
    edge *next;
  };
  // The in-counter.
  std::atomic<int> pending{1};
  // The out-set: the targets of the edges that leave this vertex, or
  // finished_mark once the vertex has notified them.
  std::atomic<edge *> out{nullptr};

  static edge *finished_mark() { return reinterpret_cast<edge *>(1); }

  void leave() override;
#if defined(MTP_STATS)
  bool is_vertex() const override { return true; }
#endif
  // Called on the vertex's stack once its body has returned: count down the
  // targets of the out-edges. A vertex may override it to represent its
  // out-edges some other way. It may call fork2join and wait. Whatever it
  // does, its last call to count_down is its last use of the vertex: the
  // target may free the vertex, or make it ready again, at once.
  //
  // Nothing requires the vertex to have left its stack first. A join and a
  // suspension guard a saved context, which no worker may resume while the
  // stack it points into is in use. A vertex's count guards only its body,
  // and the scheduler keeps nothing in a vertex while it runs.
  virtual void notify();
};

template <typename F> struct thunk_vertex : vertex {
  F body;
  explicit thunk_vertex(F body) : body(std::move(body)) {}
  void run() override { body(); }
};

// The rest of a stack, as a vertex: the stack is suspended, and resumes when
// the vertices it waits on have finished. It lives in the frame of wait.
struct suspension : vertex {
  context k;
#ifdef MTP_FIBERS
  fiber fb; // the suspended stack
#endif
#if defined(MTP_COUNT_DAG)
  size_t from = 0; // the worker that suspended the stack
#endif
  void run() override {}
  context *saved() override { return &k; }
};

/*---------------------------------------------------------------------*/
/* Work-stealing deque */

// The deque of Arora, Blumofe, and Plaxton (SPAA 1998), plus one fence in
// steal. The owner pushes and pops
// at the bottom, and thieves take from the top. When the owner empties the
// deque, it resets both ends and bumps the tag, so that a thief holding the
// old age fails its compare-and-swap.
class alignas(64) abp {
  struct alignas(8) age_t {
    uint32_t tag;
    uint32_t top;
  };
  struct alignas(64) slot { // one per cache line, to avoid false sharing
    std::atomic<task *> t; // read only below bot, so never uninitialized
  };
  // The deque holds the second branches of the calls to fork2join in
  // progress on its worker, and the vertices that its worker made ready.
  static constexpr uint32_t capacity = 1 << 16;

  std::atomic<uint32_t> bot{0};
  std::atomic<age_t> age{age_t{0, 0}};
  slot slots[capacity];

public:
  void push(task *t) {
    uint32_t b = bot.load(std::memory_order_relaxed);
    if (b == capacity) {
      std::fprintf(stderr, "microtaskparts: deque overflow\n");
      std::abort();
    }
    slots[b].t.store(t, std::memory_order_release);
    // Release is enough: the store only publishes slot b to thieves. A thief
    // may miss a task pushed just now, and try again. On x86-64, a stronger
    // store would cost a locked instruction at every fork.
    bot.store(b + 1, std::memory_order_release);
  }

  task *pop() {
    uint32_t b = bot.load(std::memory_order_relaxed);
    if (b == 0) {
      return nullptr;
    }
    b--;
    bot.store(b, std::memory_order_release);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    task *t = slots[b].t.load(std::memory_order_acquire);
    age_t old_age = age.load(std::memory_order_acquire);
    if (b > old_age.top) {
      return t; // no thief can reach slot b
    }
    // Slot b holds the last task, or was stolen. Reset the deque, and race
    // any thief for slot b.
    bot.store(0, std::memory_order_release);
    age_t new_age{old_age.tag + 1, 0};
    if ((b == old_age.top) && age.compare_exchange_strong(old_age, new_age)) {
      return t;
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
    task *t = slots[old_age.top].t.load(std::memory_order_acquire);
    age_t new_age{old_age.tag, old_age.top + 1};
    return age.compare_exchange_strong(old_age, new_age) ? t : nullptr;
  }
};

/*---------------------------------------------------------------------*/
/* Instrumentation: what each worker records */

// At any time, a worker is in one of these states.
enum state : uint32_t {
  working = 0, // running client code, or the tasks of its own deque
  scheduling,  // in its scheduler loop, preparing a task that it stole
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

// What one worker records during one call to launch. Only that worker
// writes it, and launch reads it once the worker's thread has stopped, so
// nothing here needs synchronization.
struct recorder {
  state current = scheduling;
  uint64_t since = 0;                // when the worker entered current
  uint64_t ticks_in[nb_states] = {}; // total time in each state so far
  // Counted in the loop.
  uint64_t nb_steals = 0;            // tasks stolen, of any kind
  uint64_t nb_stolen_vertices = 0;   // those that were vertices
  uint64_t nb_stolen_stacks = 0;     // those that were suspended stacks
  uint64_t nb_resumes = 0;           // suspended stacks resumed
  uint64_t nb_stacks_from_pool = 0;  // tasks started when w.stacks was empty
  // Counted in fork2join, with MTP_COUNT_FORKS.
  uint64_t nb_forks = 0;
  // Counted with MTP_COUNT_DAG, in the loop and in client code.
  uint64_t nb_vertices = 0;       // vertices started
  uint64_t nb_fork_switches = 0;  // forks that switched to the loop
  uint64_t nb_fork_foreign = 0;   // those whose pop found some other task
  uint64_t nb_edges = 0;          // edges added
  uint64_t nb_edges_late = 0;     // edges not added: the source had finished
  uint64_t nb_edge_retries = 0;   // failed compare-and-swaps on an out-set
  uint64_t nb_waits = 0;          // calls to wait
  uint64_t nb_suspends = 0;       // those that suspended their stack
  uint64_t nb_migrations = 0;     // stacks resumed by another worker
  std::deque<event> log; // grows in chunks, so appending moves no entry
};

// MTP_COUNT(w, c) adds one to count c of worker w, with MTP_COUNT_DAG, and
// otherwise evaluates nothing.
#if defined(MTP_COUNT_DAG)
#define MTP_COUNT(w, c) ((w).rec.c++)
#else
#define MTP_COUNT(w, c) ((void)0)
#endif

// MTP_COUNT_RETRY(w) is true, and counts a failed compare-and-swap of worker
// w with MTP_COUNT_DAG.
#if defined(MTP_COUNT_DAG)
#define MTP_COUNT_RETRY(w) (((w).rec.nb_edge_retries++), true)
#else
#define MTP_COUNT_RETRY(w) (true)
#endif

/*---------------------------------------------------------------------*/
/* Workers */

struct alignas(64) worker {
  // The scheduler loop runs on the worker thread's own stack. Its context is
  // saved here while the worker runs client code on a task stack.
  context loop;
  // Messages to the loop from the code that switches to it. On the way out:
  // the task to start, and its fresh stack. On the way back: the join that a
  // branch has just reached, the suspension whose stack has just been saved,
  // and the stack that has just been left for good.
  task *starting = nullptr;
  char *fresh = nullptr;
  join *reached = nullptr;
  suspension *suspended = nullptr;
  char *spent = nullptr;
#ifdef MTP_FIBERS
  fiber loop_fb;             // the stack of the loop
  fiber cur;                 // the task stack that this worker is running on
  void *dead_tsan = nullptr; // the handle of the stack that was just left
#endif
  uint64_t nb_steal_attempts = 0; // seeds the choice of victim
  // A few stacks that no task is using, so that starting a task seldom
  // takes the lock of the shared pool.
  std::vector<char *> stacks;
  abp deque;
#if defined(MTP_STATS)
  // After the deque, so that the fields above sit where they do in a build
  // with no instrumentation, and fork2join compiles to the same code. The
  // deque ends at a cache-line boundary, and so does the worker, so the
  // recorder shares no line with a deque, which thieves write to, nor with
  // another worker's recorder.
  recorder rec;
#endif

  ~worker() {
    for (char *s : stacks) {
      delete_stack(s);
    }
    delete_stack(spent); // left when the loop stopped
  }
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
    if (t->saved() != nullptr) {
      w.rec.nb_stolen_stacks++;
    } else if (t->is_vertex()) {
      w.rec.nb_stolen_vertices++;
    }
#if defined(MTP_LOG)
    w.rec.log.push_back(event{ticks(), event::steal, (uint32_t)victim});
#endif
  }
#endif
  return t;
}

// Record that worker w is now in state s. This is the only place where a
// worker reads the clock, and only the scheduler loop calls it: w is the
// worker whose loop is running, and client code never comes here. In a
// build with no instrumentation, it does nothing.
inline void mark([[maybe_unused]] worker &w, [[maybe_unused]] state s) {
#if defined(MTP_STATS)
  recorder &r = w.rec;
  if (r.current == s) {
    return; // e.g. still idle, after one more failed steal
  }
  uint64_t now = ticks();
  r.ticks_in[r.current] += now - r.since;
  r.since = now;
  r.current = s;
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

static constexpr size_t max_nb_cached_stacks = 16;

// A branch has reached its join: tell the loop, which counts it once we are
// off this stack.
template <typename F> void fork_task<F>::leave() {
  me().reached = j;
}

// With MTP_CHECK_ORPHANS: how many vertices and suspensions are ready or
// running. One is counted in when its count reaches zero, and out just
// before it notifies its successors, or when its stack resumes. So when the
// sink of launch runs, it is the only one counted, unless some vertex that
// nothing waited for has yet to finish.
#ifdef MTP_CHECK_ORPHANS
inline std::atomic<long> nb_live{0};
#endif

// One predecessor of v has finished, or v was released. If none remains, v
// is ready: make it stealable.
inline void count_down(vertex *v) {
  perturb();
  if (v->pending.fetch_sub(1, std::memory_order_acq_rel) == 1) {
#ifdef MTP_CHECK_ORPHANS
    nb_live.fetch_add(1);
#endif
    me().deque.push(v);
  }
}

// Called on the vertex's stack once its body has returned.
inline void vertex::leave() {
#ifdef MTP_CHECK_ORPHANS
  nb_live.fetch_sub(1);
#endif
  notify();
}

inline void vertex::notify() {
  // The exchange is our last use of this vertex.
  perturb();
  edge *e = out.exchange(finished_mark(), std::memory_order_acq_rel);
  while (e != nullptr) {
    edge *next = e->next;
    vertex *target = e->target;
    delete e;
    count_down(target);
    e = next;
  }
}

// Every fresh stack starts here.
[[noreturn]] inline void task_entry() {
  task *t;
  char *stack;
  {
    worker &w = me();
#ifdef MTP_FIBERS
    fiber_arrive(nullptr, &w.loop_fb);
#endif
    t = w.starting;
    stack = w.fresh;
  }
  t->run();
  perturb();
  // A fork tells the loop which join it has reached, and a vertex counts down
  // its successors. After that, t may be gone.
  t->leave();
  // run() and leave() may each return on a different worker than the one
  // that called them. Leave this stack for good, and give it to the loop.
  worker &w = me();
  w.spent = stack;
#ifdef MTP_FIBERS
  w.dead_tsan = w.cur.tsan;
  fiber_leave(nullptr, w.loop_fb);
#endif
  mtp_context_restore(&w.loop);
}

// Start a task taken out of a deque: resume it, if it is a suspended stack,
// and otherwise give it a fresh stack.
inline void start(worker &w, task *t) {
  if (context *k = t->saved()) {
#ifdef MTP_CHECK_ORPHANS
    nb_live.fetch_sub(1);
#endif
#if defined(MTP_STATS)
    w.rec.nb_resumes++;
#endif
#ifdef MTP_FIBERS
    void *save;
    w.cur = static_cast<suspension *>(t)->fb;
    fiber_leave(&save, w.cur);
#endif
    mark(w, working);
    mtp_context_swap(&w.loop, k);
#ifdef MTP_FIBERS
    fiber_arrive(save);
#endif
    return;
  }
  char *stack;
#if defined(MTP_COUNT_DAG)
  // Asking a task for its kind costs more than the rest of the totals put
  // together, so it is done only in a build that counts.
  if (t->is_vertex()) {
    w.rec.nb_vertices++;
  }
#endif
#if defined(MTP_STATS)
  if (w.stacks.empty()) {
    w.rec.nb_stacks_from_pool++;
  }
#endif
  if (w.stacks.empty()) {
    stack = take_stack();
  } else {
    stack = w.stacks.back();
    w.stacks.pop_back();
  }
  stack_taken(stack);
  // The context is read once, by the switch, so it can live in this frame.
  context k;
  context_init(k, stack, stack_szb, &task_entry);
  w.starting = t;
  w.fresh = stack;
#ifdef MTP_FIBERS
  void *save;
  w.cur.bottom = stack;
  w.cur.szb = stack_szb;
#ifdef MTP_TSAN
  w.cur.tsan = __tsan_create_fiber(0);
#endif
  fiber_leave(&save, w.cur);
#endif
  mark(w, working);
  mtp_context_swap(&w.loop, &k);
#ifdef MTP_FIBERS
  fiber_arrive(save);
#endif
}

// Runs on the worker thread's own stack and never migrates, so w stays
// valid. first, if not null, is a task to start with.
//
// The loop is the only place where a join is counted down and where a
// suspension is released. The code concerned first switches here, so by the time
// another worker can act on a count that reached zero, that code is off its
// stack, and the context that the other worker may resume has been saved.
//
// With MTP_STATS, the loop records a change of state only when the worker's
// own deque is empty. While it finds tasks there, the worker stays recorded
// as working, and reads no clock.
inline void worker_loop(task *first) {
  worker &w = me();
#ifdef MTP_TSAN
  w.loop_fb.tsan = __tsan_get_current_fiber();
#endif
  task *t = first;
  while (!done.load()) {
#if defined(MTP_MARK_EVERY_SWITCH)
    if (w.rec.current == working) {
      mark(w, scheduling);
    }
#endif
    // Messages from the stack that we just left.
    if (char *stack = w.spent) {
      w.spent = nullptr;
#ifdef MTP_TSAN
      __tsan_destroy_fiber(w.dead_tsan);
      w.dead_tsan = nullptr;
#endif
      stack_given_back(stack);
      if (w.stacks.size() < max_nb_cached_stacks) {
        w.stacks.push_back(stack);
      } else {
        put_stack(stack);
      }
    }
    if (join *j = w.reached) {
      w.reached = nullptr;
      perturb();
      if (j->pending.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        // Both branches are done: resume the parent.
#ifdef MTP_FIBERS
        void *save;
        w.cur = j->fb;
        fiber_leave(&save, w.cur);
#endif
        mark(w, working);
        mtp_context_swap(&w.loop, &j->k);
#ifdef MTP_FIBERS
        fiber_arrive(save);
#endif
        continue;
      }
    }
    if (suspension *s = w.suspended) {
      w.suspended = nullptr;
      count_down(s);
    }
    // Look for a task: first in our own deque, then in another worker's.
    if (t == nullptr) {
      t = w.deque.pop();
    }
    if ((t == nullptr) && (nb_workers > 1)) {
      t = steal(w);
#if defined(MTP_STATS)
      if (t != nullptr) {
        mark(w, scheduling);
      }
#endif
    }
    if (t == nullptr) {
      mark(w, idle);
      std::this_thread::yield();
      continue;
    }
    start(w, t);
    // Back on our own stack.
    t = nullptr;
  }
  mark(w, stopped);
}

/*---------------------------------------------------------------------*/
/* Instrumentation: what a call to launch cost */

// When the last call to launch started and ended. It starts before the
// worker threads do, and ends once they have all stopped.
struct launch_times {
  std::chrono::steady_clock::time_point wall_start, wall_end;
  uint64_t start = 0;
  uint64_t end = 0;
};
inline launch_times last_launch;

// The counts that launch_stats sums over the recorders.
#define MTP_FOR_EACH_COUNT(X)                                                 \
  X(nb_steals) X(nb_stolen_vertices) X(nb_stolen_stacks) X(nb_vertices)       \
  X(nb_resumes) X(nb_stacks_from_pool) X(nb_forks) X(nb_fork_switches)        \
  X(nb_fork_foreign) X(nb_edges) X(nb_edges_late) X(nb_edge_retries)          \
  X(nb_waits) X(nb_suspends) X(nb_migrations)

// What the last call to launch cost, with times in seconds. The three times
// are summed over the workers, and add up to nb_workers * exectime. All is
// zero in a build with no instrumentation.
struct stats {
  size_t nb_workers = 0;
  double exectime = 0.0; // wall-clock time
  double work = 0.0;
  double sched = 0.0;
  double idle = 0.0;
  uint64_t nb_stacks_mapped = 0; // since the program started
#define MTP_X(c) uint64_t c = 0;
  MTP_FOR_EACH_COUNT(MTP_X)
#undef MTP_X
};

// How long a tick lasted, on average, during the last call to launch.
inline double seconds_per_tick() {
  const launch_times &l = last_launch;
  std::chrono::duration<double> elapsed = l.wall_end - l.wall_start;
  return (l.end == l.start) ? 0.0 : elapsed.count() / (double)(l.end - l.start);
}

// Called after launch has returned, and before the next call to it.
inline stats launch_stats() {
  stats s;
#if defined(MTP_STATS)
  const launch_times &l = last_launch;
  double spt = seconds_per_tick();
  uint64_t in[nb_states] = {};
  for (size_t i = 0; i < nb_workers; i++) {
    const recorder &r = workers[i].rec;
    for (int k = 0; k < nb_states; k++) {
      in[k] += r.ticks_in[k];
    }
    // From the moment the worker stopped until every thread had: it had
    // nothing to do.
    in[idle] += l.end - r.since;
#define MTP_X(c) s.c += r.c;
    MTP_FOR_EACH_COUNT(MTP_X)
#undef MTP_X
  }
  s.nb_workers = nb_workers;
  s.exectime = std::chrono::duration<double>(l.wall_end - l.wall_start).count();
  s.work = (double)in[working] * spt;
  s.sched = (double)in[scheduling] * spt;
  s.idle = (double)in[idle] * spt;
  s.nb_stacks_mapped = nb_stacks_mapped.load();
#endif
  return s;
}

// One line per group of values, each a name and a number. A build prints
// only what it records.
inline void print_stats(FILE *f, [[maybe_unused]] const stats &s) {
#if defined(MTP_STATS)
  auto n = [](uint64_t x) { return (unsigned long long)x; };
  std::fprintf(f, "states    work %.6f sched %.6f idle %.6f s\n", s.work,
               s.sched, s.idle);
  std::fprintf(f,
               "tasks     steals %llu stolen_vertices %llu "
               "stolen_stacks %llu resumes %llu\n",
               n(s.nb_steals), n(s.nb_stolen_vertices),
               n(s.nb_stolen_stacks), n(s.nb_resumes));
  std::fprintf(f, "stacks    mapped %llu from_pool %llu\n",
               n(s.nb_stacks_mapped), n(s.nb_stacks_from_pool));
#if defined(MTP_COUNT_FORKS)
  std::fprintf(f, "forks     %llu\n", n(s.nb_forks));
#endif
#if defined(MTP_COUNT_DAG)
  std::fprintf(f,
               "dag       vertices %llu edges %llu late %llu retries %llu "
               "waits %llu suspends %llu migrations %llu fork_switches %llu "
               "fork_foreign %llu\n",
               n(s.nb_vertices), n(s.nb_edges), n(s.nb_edges_late), n(s.nb_edge_retries),
               n(s.nb_waits), n(s.nb_suspends), n(s.nb_migrations),
               n(s.nb_fork_switches), n(s.nb_fork_foreign));
#endif
#endif
}

#if defined(MTP_LOG)
// Write the log of the last call to launch in the JSON trace format that
// Perfetto (ui.perfetto.dev) reads: one track per worker, one slice per stay
// in a state, and one instant per steal. Times are in microseconds from the
// start of the call. Called where launch_stats may be.
inline void write_trace(const char *path) {
  FILE *f = std::fopen(path, "w");
  if (f == nullptr) {
    std::perror(path);
    return;
  }
  const launch_times &l = last_launch;
  double spt = seconds_per_tick();
  auto us = [&](uint64_t t) { return (double)(t - l.start) * spt * 1e6; };
  std::fprintf(f, "{\"displayTimeUnit\": \"ns\", \"traceEvents\": [\n");
  for (size_t i = 0; i < nb_workers; i++) {
    std::fprintf(f,
                 "%s{\"name\": \"thread_name\", \"ph\": \"M\", \"pid\": 0, "
                 "\"tid\": %zu, \"args\": {\"name\": \"worker %zu\"}}",
                 (i == 0) ? "" : ",\n", i, i);
    // A stay in a state, from one entry to the next.
    auto slice = [&](const event &from, uint64_t until) {
      if (from.ticks < until) {
        std::fprintf(f,
                     ",\n{\"name\": \"%s\", \"ph\": \"X\", \"pid\": 0, "
                     "\"tid\": %zu, \"ts\": %.3f, \"dur\": %.3f}",
                     state_names[from.what], i, us(from.ticks),
                     us(until) - us(from.ticks));
      }
    };
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
        slice(*open, e.ticks);
      }
      open = &e;
    }
    if (open != nullptr) {
      slice(*open, l.end);
    }
  }
  std::fprintf(f, "\n]}\n");
  std::fclose(f);
}
#endif

// Run f, on nb_workers threads; the caller is worker 0. f is the body of a
// vertex, the root, and the computation ends when a second vertex runs, the
// sink, which waits for the root and stops the workers. So launch needs no
// case of its own in the scheduler: it ends the way any vertex follows
// another. A vertex that the root releases and nothing waits for may not
// have run by then, and is dropped.
template <typename F> __attribute__((noinline)) void launch(F &&f) {
  const char *s = std::getenv("TASKPARTS_NUM_WORKERS");
  nb_workers = (s != nullptr)
                   ? std::max(1, std::atoi(s))
                   : std::max(1u, std::thread::hardware_concurrency());
  workers.reset(new worker[nb_workers]);
#if defined(MTP_STATS)
  // Every worker starts out scheduling, so the time it takes to start a
  // thread counts as scheduling.
  last_launch.wall_start = std::chrono::steady_clock::now();
  last_launch.start = ticks();
  for (size_t i = 0; i < nb_workers; i++) {
    recorder &r = workers[i].rec;
    r.since = last_launch.start;
#if defined(MTP_LOG)
    r.log.push_back(event{r.since, scheduling, 0});
#endif
  }
#endif
  done.store(false);
  in_scheduler = true;
  tls_id = 0;
  auto run_root = [&] { f(); };
  auto stop = [] {
#ifdef MTP_CHECK_ORPHANS
    // Every vertex but this one has notified its successors, if each task
    // waited for what it made ready.
    if (nb_live.load() != 1) {
      std::fprintf(stderr,
                   "microtaskparts: the computation ended while %ld "
                   "vertices that nothing waited for were ready or running\n",
                   nb_live.load() - 1);
      std::abort();
    }
#endif
    done.store(true);
  };
#ifdef MTP_CHECK_ORPHANS
  nb_live.store(0);
#endif
  thunk_vertex<decltype(run_root)> root(run_root);
  thunk_vertex<decltype(stop)> sink(stop);
  new_edge(&root, &sink);
  release(&sink);
  release(&root); // now in worker 0's deque
  std::vector<std::thread> threads;
  for (size_t i = 1; i < nb_workers; i++) {
    threads.emplace_back([i] {
      tls_id = i;
      worker_loop(nullptr);
    });
  }
  worker_loop(nullptr);
  for (auto &t : threads) {
    t.join();
  }
#if defined(MTP_STATS)
  last_launch.end = ticks();
  last_launch.wall_end = std::chrono::steady_clock::now();
#endif
  for (char *stack : stack_pool) {
    delete_stack(stack);
  }
  stack_pool.clear();
  in_scheduler = false;
}

/*---------------------------------------------------------------------*/
/* Fork join */

// Run f1() and f2(), possibly in parallel, and return when both are done.
// The numbered steps match the figure of ../main.tex that defines fork2join.
template <typename F1, typename F2> void fork2join(F1 &&f1, F2 &&f2) {
  if (!in_scheduler) {
    launch([&] { fork2join(f1, f2); });
    return;
  }
  join j;
  fork_task<F2> own(f2, &j);

  // (1) Fork: make the second branch stealable, and run the first.
  worker &w1 = me();
  MTP_ON_FORK(w1);
  w1.deque.push(&own);
  f1();
  perturb();

  // (2) Try to take the second branch back. f1 may have returned on another
  // worker, so look the worker up again.
  worker &w = me();
  task *t = w.deque.pop();
  if (t == &own) {
    // (3) Not stolen: run the second branch here, as an ordinary call.
    f2();
    return;
  }
  // We popped some other task t: a vertex or a suspension that became ready
  // and was pushed on this deque above own, or below which own never was.
  // Put it back. Either way, own is not ours to run: a thief has it, or it
  // is still in a deque, and whichever worker takes it out will start it on
  // a fresh stack.
  MTP_COUNT(w, nb_fork_switches);
  if (t != nullptr) {
    MTP_COUNT(w, nb_fork_foreign);
    w.deque.push(t);
  }

  // (4) Suspend this stack at the join, and switch to the scheduler loop,
  // which counts this branch. Whichever branch is counted last resumes
  // us here, on its worker.
  w.reached = &j;
#ifdef MTP_FIBERS
  void *save;
  j.fb = w.cur;
  fiber_leave(&save, w.loop_fb);
#endif
  mtp_context_swap(&j.k, &w.loop);
#ifdef MTP_FIBERS
  fiber_arrive(save, &me().loop_fb);
#endif
}

/*---------------------------------------------------------------------*/
/* DAG calculus */

// The four operations below must be called from inside launch.

// Make a vertex that runs f. The caller owns it, adds the edges that enter
// it, and then releases it.
template <typename F> vertex *new_vertex(F f) {
  return new thunk_vertex<F>(std::move(f));
}

// Make dst wait for src. The caller must hold the count of dst above zero:
// either dst has not been released yet, or the caller is a vertex with an
// edge to dst, and is still running. Returns false, and adds nothing, if src
// has already finished.
inline bool new_edge(vertex *src, vertex *dst) {
  // Count the edge before a concurrent notify() of src can see it.
  dst->pending.fetch_add(1, std::memory_order_acq_rel);
  perturb();
  auto e = new vertex::edge{dst, nullptr};
  vertex::edge *head = src->out.load(std::memory_order_acquire);
  do {
    if (head == vertex::finished_mark()) {
      delete e;
      // Cannot reach zero: the caller holds the count of dst above zero.
      dst->pending.fetch_sub(1, std::memory_order_acq_rel);
      MTP_COUNT(me(), nb_edges_late);
      return false;
    }
    e->next = head;
  } while (!src->out.compare_exchange_weak(head, e, std::memory_order_acq_rel) &&
           MTP_COUNT_RETRY(me()));
  MTP_COUNT(me(), nb_edges);
  return true;
}

// Let v run once its predecessors have finished.
inline void release(vertex *v) { count_down(v); }

// Return once v has finished. The rest of the caller's stack becomes a
// vertex that waits for v: the stack is suspended, and whichever worker
// takes the suspension out of a deque resumes it.
inline void wait(vertex *v) {
  assert(in_scheduler);
  suspension s;
  MTP_COUNT(me(), nb_waits);
  if (!new_edge(v, &s)) {
    return; // v has already finished
  }
  // Switch to the scheduler loop, which releases s once this stack is saved.
  perturb();
  worker &w = me();
  MTP_COUNT(w, nb_suspends);
#if defined(MTP_COUNT_DAG)
  s.from = &w - &workers[0];
#endif
  w.suspended = &s;
#ifdef MTP_FIBERS
  void *save;
  s.fb = w.cur;
  fiber_leave(&save, w.loop_fb);
#endif
  mtp_context_swap(&s.k, &w.loop);
#ifdef MTP_FIBERS
  fiber_arrive(save, &me().loop_fb);
#endif
#if defined(MTP_COUNT_DAG)
  {
    worker &w2 = me();
    if ((size_t)(&w2 - &workers[0]) != s.from) {
      w2.rec.nb_migrations++;
    }
  }
#endif
}

} // namespace microtaskparts
