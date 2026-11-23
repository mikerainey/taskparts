#pragma once

// microtaskparts, with elastic scheduling.
//
// A teaching companion to ../main.tex. It is the micro taskparts of
// ../../native-fork-join, plus the elastic scheduling of that paper: a worker
// that finds nothing to steal goes to sleep, and a worker that makes work
// wakes one up. What is added is the section "Elastic scheduling" below, and
// a few lines elsewhere, each marked "Elastic": in the deque's push, in finish,
// in worker_loop, in launch, and in fork2join.
//
// It is not a runtime for applications: each top-level call to fork2join
// starts and stops its own worker threads. ../../parlay-adapter runs ParlayLib
// programs on it, by running the whole of main in one call to launch.
//
// Instrumentation, as in ../../instrumentation, is off unless a build turns
// it on (Appendix A of ../main.tex):
//
//   -DMTP_STATS        time each worker spends working, scheduling, idle and
//                      asleep, and count steals, sleeps and wake-ups
//   -DMTP_COUNT_FORKS  MTP_STATS, plus a count of forks
//
// A program measures a region of one call to launch by taking a snapshot
// before it and another after it (take_snapshot, stats_between).

#include "context.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <sys/mman.h>
#include <unistd.h>

#if defined(MTP_COUNT_FORKS) && !defined(MTP_STATS)
#define MTP_STATS
#endif

// On x86-64, a store followed by a load of another location needs a locked
// instruction to keep the two in order, and Linux lets the thread on the other
// side of the race pay for the ordering instead (see sleep, below).
#if defined(__linux__) && defined(__x86_64__)
#define MTP_REMOTE_BARRIER 1
#include <linux/membarrier.h>
#include <sys/syscall.h>
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

/*---------------------------------------------------------------------*/
/* Stacks of stolen tasks */

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
// process, so the stacks of finished tasks are kept for reuse (take_stack and
// put_stack, after the workers).
inline char *new_stack() {
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

// The stacks of stolen tasks that have finished, kept for the next stolen
// tasks. All workers share this pool, under a lock. Each worker also keeps a
// few stacks of its own (below), and comes here only when it has none to
// take or no room to keep one: with the pool alone, a program that steals
// hundreds of thousands of times a second spends its time waiting for the
// lock. A thief that always finishes before its victim reaches the join
// takes stacks and never returns one, and its victim the reverse, which is
// why the workers' own stacks are not enough without the pool.
inline std::mutex stack_pool_lock;
inline std::vector<char *> stack_pool;

// How many released stacks a worker keeps for itself.
static constexpr size_t stack_cache_cap = 16;

// The second branch of a call to fork2join, which thieves may steal. It
// lives in the frame of that call too.
struct task {
  join *j;               // the join to report to; null for the root
  char *stack = nullptr; // the fresh stack it runs on, if it is stolen
  context k;             // where it starts on that stack

  // No destructor: one would run at every return from fork2join, stolen or
  // not. Whoever waits for a stolen task releases its stack.
  explicit task(join *j) : j(j) {}
  virtual void run() = 0;
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
    // Not initialized: a slot is read only below bot, after a push has
    // written it, and writing every slot would make each launch touch the
    // whole deque of every worker.
    std::atomic<task *> v;
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
    // Elastic: fork2join reads the status word next, and a worker that is
    // about to sleep does the opposite: it writes the status word, then reads
    // bot (may_have_task, below). One of the two must see the other, so this
    // store must take effect before that read. A sequentially consistent
    // store ensures it. On AArch64 it is the same instruction as the release
    // store that suffices to publish slot b to thieves. On x86-64 it is a
    // locked instruction at every fork, so there the worker about to sleep
    // orders the two instead (remote_barrier).
#ifdef MTP_REMOTE_BARRIER
    bot.store(b + 1, std::memory_order_release);
#else
    bot.store(b + 1, std::memory_order_seq_cst);
#endif
  }

  // Whether the deque holds a task, for a worker about to sleep. An answer
  // of true can be stale, and false is exact as of some moment after the
  // caller's last sequentially consistent operation.
  bool may_have_task() {
    uint32_t b = bot.load(std::memory_order_seq_cst);
    return b > age.load(std::memory_order_seq_cst).top;
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

// At any time between the start and the end of a call to launch, a worker is
// in one of these states.
enum state : uint32_t {
  working = 0, // running client code, on a task stack
  scheduling,  // in its scheduler loop, with something to do
  idle,        // awake, and failing to steal
  asleep,      // waiting for a token
  nb_states
};

// The processor's cycle counter, or the nearest thing to it: one
// instruction, and no call. Two snapshots tell how long a tick lasts.
inline uint64_t ticks() {
#if defined(__aarch64__)
  uint64_t t;
  asm volatile("mrs %0, cntvct_el0" : "=r"(t));
  return t;
#else
  return __builtin_ia32_rdtsc();
#endif
}

// What one worker records during a call to launch: nothing here is reset.
// Only that worker writes it. Another thread reads it only when it has seen
// the worker asleep with no task anywhere (quiesce, below): a worker that is
// asleep writes nothing here until a token wakes it, and then nobody posts
// one. So only current, through which the worker says that it is asleep, is
// atomic.
struct recorder {
  std::atomic<state> current{scheduling};
  uint64_t since = 0;                // when the worker entered current
  uint64_t ticks_in[nb_states] = {}; // total time in each state so far
  uint64_t nb_steals = 0;
  uint64_t nb_forks = 0;
  uint64_t nb_sleeps = 0;      // times the worker waited for a token
  uint64_t nb_cancels = 0;     // announcements it took back: a task was there
  uint64_t nb_fork_wakes = 0;  // workers it woke at a fork
  uint64_t nb_thief_wakes = 0; // workers it woke after stealing
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
  // Released stacks that this worker keeps; only this worker touches them.
  char *stacks[stack_cache_cap];
  size_t nb_stacks = 0;
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

// Record that worker w is now in state s. This is the only place where a
// worker reads the clock, and only the scheduler loop and what it calls come
// here: w is the worker whose loop is running, and client code never does.
inline void mark([[maybe_unused]] worker &w, [[maybe_unused]] state s) {
#if defined(MTP_STATS)
  recorder &r = w.rec;
  state was = r.current.load(std::memory_order_relaxed);
  if (was == s) {
    return; // e.g. still idle, after one more failed steal
  }
  uint64_t now = ticks();
  r.ticks_in[was] += now - r.since;
  r.since = now;
  // Last, so that whoever sees the new state sees the writes above.
  r.current.store(s, std::memory_order_release);
#endif
}

// Add one to a count of worker w's recorder; nothing, without instrumentation.
#if defined(MTP_STATS)
#define MTP_COUNT(w, field) ((w).rec.field++)
#else
#define MTP_COUNT(w, field) ((void)0)
#endif

// The hook that fork2join runs on each fork, on the worker w that makes the
// fork. A program may define its own before including this file.
#if !defined(MTP_ON_FORK)
#if defined(MTP_COUNT_FORKS)
#define MTP_ON_FORK(w) ((w).rec.nb_forks++)
#else
#define MTP_ON_FORK(w) ((void)(w))
#endif
#endif

// A stack for a task that w stole: one of its own, or one from the pool, or
// a fresh one.
inline char *take_stack(worker &w) {
  if (w.nb_stacks > 0) {
    return w.stacks[--w.nb_stacks];
  }
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

// Release the stack of a finished stolen task: the calling worker keeps it,
// or hands it to the pool. Kept out of line, like launch: it runs only after
// a steal.
__attribute__((noinline)) inline void put_stack(char *stack) {
  worker &w = me();
  if (w.nb_stacks < stack_cache_cap) {
    w.stacks[w.nb_stacks++] = stack;
    return;
  }
  std::lock_guard<std::mutex> guard(stack_pool_lock);
  stack_pool.push_back(stack);
}

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
/* Elastic scheduling */

// A counting semaphore: wait blocks its thread until a token is posted, and
// takes it.
class semaphore {
  std::mutex m;
  std::condition_variable cv;
  long nb_tokens = 0;

public:
  void post() {
    {
      std::lock_guard<std::mutex> guard(m);
      nb_tokens++;
    }
    cv.notify_one();
  }
  void wait() {
    std::unique_lock<std::mutex> guard(m);
    cv.wait(guard, [&] { return nb_tokens > 0; });
    nb_tokens--;
  }
};

// The status word counts the workers that are asleep, in its low half, and
// the workers that are stealing, in its high half. A worker that is neither
// is working, or counting a branch in its loop. Only a worker that is stealing,
// or one that has seen the need for a thief, writes the word; a fork only reads
// it.
struct alignas(64) status_word {
  std::atomic<uint32_t> v{0};
};
inline status_word status;
inline semaphore sleepers; // where the workers that are asleep wait

static constexpr uint32_t one_asleep = 1;
static constexpr uint32_t one_stealing = 1u << 16;

inline uint32_t nb_asleep(uint32_t s) { return s & 0xffff; }

// Whether some worker is asleep and none is stealing.
inline bool needs_thief(uint32_t s) { return (s - 1) < 0xffff; }

// How many steals in a row a thief fails before it goes to sleep.
#ifndef MTP_PATIENCE
#define MTP_PATIENCE 1000
#endif
inline uint64_t patience = MTP_PATIENCE;

// Wake one worker, if some worker is asleep and none is stealing. The waker
// moves the count of the sleeper from asleep to stealing on its behalf, so
// that no second waker wakes another for the same task. Kept out of line: a
// fork comes here only when it has seen the need. Returns whether it woke one.
__attribute__((noinline)) inline bool ensure_thief() {
  uint32_t s = status.v.load();
  while (needs_thief(s)) {
    if (status.v.compare_exchange_weak(s, s - one_asleep + one_stealing)) {
      sleepers.post();
      return true;
    }
  }
  return false;
}

// Wake every worker that is asleep: the computation is over.
inline void wake_all() {
  uint32_t s = status.v.load();
  while (nb_asleep(s) > 0) {
    if (status.v.compare_exchange_weak(s, s - one_asleep + one_stealing)) {
      sleepers.post();
    }
  }
}

inline bool exists_task() {
  for (size_t i = 0; i < nb_workers; i++) {
    if (workers[i].deque.may_have_task()) {
      return true;
    }
  }
  return false;
}

// Make every thread of the process that is running execute a memory barrier
// before this returns: any store that such a thread made before a load it has
// already made is visible to the caller afterwards. Needed only where push
// does not order its store itself.
inline void remote_barrier_init() {
#ifdef MTP_REMOTE_BARRIER
  if (syscall(SYS_membarrier, MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED, 0, 0) != 0) {
    std::perror("microtaskparts: membarrier");
    std::abort();
  }
#endif
}

inline void remote_barrier() {
#ifdef MTP_REMOTE_BARRIER
  if (syscall(SYS_membarrier, MEMBARRIER_CMD_PRIVATE_EXPEDITED, 0, 0) != 0) {
    std::perror("microtaskparts: membarrier");
    std::abort();
  }
#endif
}

// Go to sleep, unless there is a task to steal or the computation is over.
// The caller is counted as stealing, and is again when this returns. First
// announce, then look: a fork pushes, then reads the announcement.
inline void sleep([[maybe_unused]] worker &w) {
  status.v.fetch_add(one_asleep - one_stealing);
  remote_barrier();
  if (done.load() || exists_task()) {
    // Take the announcement back, unless a waker has already taken every
    // announcement, ours included: then a token is on its way to us.
    uint32_t s = status.v.load();
    while (nb_asleep(s) > 0) {
      if (status.v.compare_exchange_weak(s, s - one_asleep + one_stealing)) {
        MTP_COUNT(w, nb_cancels);
        return;
      }
    }
  }
  MTP_COUNT(w, nb_sleeps);
  mark(w, asleep);
  sleepers.wait();
  mark(w, idle);
}

// Steal a task for worker w, sleeping when there is none to be found. Returns
// null only when the computation is over.
__attribute__((noinline)) inline task *acquire(worker &w) {
  status.v.fetch_add(one_stealing);
  task *t = nullptr;
  uint64_t nb_failed = 0;
  while (!done.load() && ((t = steal(w)) == nullptr)) {
    mark(w, idle);
    if (++nb_failed < patience) {
      std::this_thread::yield();
    } else {
      nb_failed = 0;
      sleep(w);
    }
  }
  uint32_t s = status.v.fetch_sub(one_stealing) - one_stealing;
  // We were perhaps the last thief, and the deque we stole from may hold more.
  if (t != nullptr) {
    MTP_COUNT(w, nb_steals);
    if (needs_thief(s) && ensure_thief()) {
      MTP_COUNT(w, nb_thief_wakes);
    }
  }
  return t;
}

/*---------------------------------------------------------------------*/
/* Scheduler loop */

// A branch running on worker w has reached join j (null for the root). Leave
// the current stack for good and tell the loop, which counts the branch only
// once we are off the stack.
[[noreturn]] inline void finish(worker &w, join *j) {
  if (j == nullptr) {
    done.store(true);
    wake_all(); // Elastic
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
// releases for reuse before fork2join returns, for the second.
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
               ((nb_workers > 1) && ((t = acquire(w)) != nullptr))) { // Elastic
      mark(w, scheduling);
      t->stack = take_stack(w);
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
}

/*---------------------------------------------------------------------*/
/* Instrumentation: measuring a region */

// A program measures a region of its execution by taking a snapshot before
// it and another after it. A snapshot is taken by the root task, in its
// sequential code: inside a call to launch, and outside every call to
// fork2join. The recorders are never reset: what a region cost is the
// difference of two snapshots.

// Wait until every other worker is asleep. No task exists, so no fork and
// no thief posts a token: the status word then shows every other worker
// asleep and none stealing, and stays so. A worker that the word shows asleep
// may still be on its way to its wait, so wait for each to say that it is
// there; from then on it writes nothing to its recorder.
inline void quiesce() {
#if defined(MTP_STATS)
  const uint32_t all_asleep = (uint32_t)(nb_workers - 1) * one_asleep;
  while (status.v.load() != all_asleep) {
    std::this_thread::yield();
  }
  size_t my_id = get_my_id();
  for (size_t i = 0; i < nb_workers; i++) {
    while ((i != my_id) &&
           (workers[i].rec.current.load(std::memory_order_acquire) != asleep)) {
      std::this_thread::yield();
    }
  }
#endif
}

// The totals of all workers, from the start of the call to launch up to one
// instant, in ticks. The counts are zero unless the build records them.
struct snapshot {
  std::chrono::steady_clock::time_point wall;
  uint64_t at = 0; // the instant, in ticks
  uint64_t ticks_in[nb_states] = {};
  uint64_t nb_steals = 0;
  uint64_t nb_forks = 0;
  uint64_t nb_sleeps = 0;
  uint64_t nb_cancels = 0;
  uint64_t nb_fork_wakes = 0;
  uint64_t nb_thief_wakes = 0;
};

inline snapshot take_snapshot() {
  if (!in_scheduler) {
    std::fprintf(stderr, "microtaskparts: snapshot outside launch\n");
    std::abort();
  }
  snapshot s;
  quiesce();
  s.wall = std::chrono::steady_clock::now();
  s.at = ticks();
#if defined(MTP_STATS)
  for (size_t i = 0; i < nb_workers; i++) {
    const recorder &r = workers[i].rec;
    for (int k = 0; k < nb_states; k++) {
      s.ticks_in[k] += r.ticks_in[k];
    }
    // The stay that the worker is in the middle of: the caller's worker is
    // working, and the others are asleep.
    s.ticks_in[r.current.load()] += s.at - r.since;
    s.nb_steals += r.nb_steals;
    s.nb_forks += r.nb_forks;
    s.nb_sleeps += r.nb_sleeps;
    s.nb_cancels += r.nb_cancels;
    s.nb_fork_wakes += r.nb_fork_wakes;
    s.nb_thief_wakes += r.nb_thief_wakes;
  }
#endif
  return s;
}

// What happened between two snapshots, with times in seconds. The four
// times are summed over the workers, and add up to nb_workers * exectime.
struct stats {
  size_t nb_workers = 0;
  double exectime = 0.0; // wall-clock time
  double work = 0.0;
  double sched = 0.0;
  double idle = 0.0;
  double asleep = 0.0;
  uint64_t nb_steals = 0;
  uint64_t nb_forks = 0;
  uint64_t nb_sleeps = 0;
  uint64_t nb_cancels = 0;
  uint64_t nb_fork_wakes = 0;
  uint64_t nb_thief_wakes = 0;
};

inline stats stats_between(const snapshot &a, const snapshot &b) {
  std::chrono::duration<double> elapsed = b.wall - a.wall;
  // How long a tick lasted, on average, between the two snapshots.
  double spt = (b.at == a.at) ? 0.0 : elapsed.count() / (double)(b.at - a.at);
  auto seconds = [&](state k) {
    return (double)(b.ticks_in[k] - a.ticks_in[k]) * spt;
  };
  stats s;
  s.nb_workers = nb_workers;
  s.exectime = elapsed.count();
  s.work = seconds(working);
  s.sched = seconds(scheduling);
  s.idle = seconds(idle);
  s.asleep = seconds(asleep);
  s.nb_steals = b.nb_steals - a.nb_steals;
  s.nb_forks = b.nb_forks - a.nb_forks;
  s.nb_sleeps = b.nb_sleeps - a.nb_sleeps;
  s.nb_cancels = b.nb_cancels - a.nb_cancels;
  s.nb_fork_wakes = b.nb_fork_wakes - a.nb_fork_wakes;
  s.nb_thief_wakes = b.nb_thief_wakes - a.nb_thief_wakes;
  return s;
}

inline void print_stats(FILE *f, const stats &s) {
  std::fprintf(f,
               "{\"nb_workers\": %zu, \"exectime\": %.6f, \"work\": %.6f, "
               "\"sched\": %.6f, \"idle\": %.6f, \"asleep\": %.6f, "
               "\"nb_steals\": %llu, \"nb_forks\": %llu, \"nb_sleeps\": %llu, "
               "\"nb_cancels\": %llu, \"nb_fork_wakes\": %llu, "
               "\"nb_thief_wakes\": %llu}\n",
               s.nb_workers, s.exectime, s.work, s.sched, s.idle, s.asleep,
               (unsigned long long)s.nb_steals, (unsigned long long)s.nb_forks,
               (unsigned long long)s.nb_sleeps, (unsigned long long)s.nb_cancels,
               (unsigned long long)s.nb_fork_wakes,
               (unsigned long long)s.nb_thief_wakes);
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
#if defined(MTP_STATS)
  // Every worker starts out scheduling, so the time it takes to start a
  // thread counts as scheduling.
  for (size_t i = 0; i < nb_workers; i++) {
    workers[i].rec.since = ticks();
  }
#endif
  done.store(false);
  // Elastic: the patience, and the remote barrier where it is used.
  if (const char *e = std::getenv("TASKPARTS_ELASTIC_PATIENCE")) {
    patience = static_cast<uint64_t>(std::max(1ll, std::atoll(e)));
  }
  remote_barrier_init();
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
  delete_stack(root.stack);
  for (size_t i = 0; i < nb_workers; i++) {
    for (size_t k = 0; k < workers[i].nb_stacks; k++) {
      delete_stack(workers[i].stacks[k]);
    }
  }
  for (char *stack : stack_pool) {
    delete_stack(stack);
  }
  stack_pool.clear();
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
  worker &w1 = me();
  MTP_ON_FORK(w1);
  w1.deque.push(&t);
  if (needs_thief(status.v.load())) { // Elastic
    if (ensure_thief()) {
      MTP_COUNT(me(), nb_fork_wakes);
    }
  }
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

  // Both branches are done, and the second has left the stack it ran on.
  put_stack(t.stack);
}

} // namespace microtaskparts
