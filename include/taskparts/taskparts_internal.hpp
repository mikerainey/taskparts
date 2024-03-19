#pragma once

#include <functional>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <string>
#include <chrono>
#include <mutex>
#include <cstdarg>

#ifdef TASKPARTS_USE_VALGRIND
// nix-build '<nixpkgs>' -A valgrind.dev
#include <valgrind/valgrind.h>
#endif

#if !defined(TASKPARTS_DARWIN) && !defined(TASKPARTS_POSIX)
#define TASKPARTS_POSIX
#endif
#if !defined(TASKPARTS_ARM64) && !defined(TASKPARTS_X64)
#define TASKPARTS_X64
#endif

/*---------------------------------------------------------------------*/
/* Diagnostics */

namespace taskparts {
std::mutex print_mutex;

static
void afprintf(FILE* stream, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  { std::unique_lock<std::mutex> lk(print_mutex);
    vfprintf(stream, fmt, ap);
    fflush(stream);
  }
  va_end(ap);
}

auto get_my_id() -> size_t;

static
void aprintf(const char *fmt, ...) {
  { std::unique_lock<std::mutex> lk(print_mutex);
    fprintf(stdout, "[%lu] ", get_my_id());
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    fflush(stdout);
    va_end(ap);
  }
}

static
void die(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  { std::unique_lock<std::mutex> lk(print_mutex);
    fprintf(stderr, "Fatal error -- ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    fflush(stderr);
  }
  va_end(ap);
  assert(false);
  exit(-1);
}
}

namespace taskparts {

/*---------------------------------------------------------------------*/
/* System clock */

static inline
auto now() -> std::chrono::time_point<std::chrono::steady_clock> {
  return std::chrono::steady_clock::now();
}

static inline
auto diff(std::chrono::time_point<std::chrono::steady_clock> start,
          std::chrono::time_point<std::chrono::steady_clock> finish) -> double {
  std::chrono::duration<double> elapsed = finish - start;
  return elapsed.count();
}

static inline
auto since(std::chrono::time_point<std::chrono::steady_clock> start) -> double {
  return diff(start, now());
}
  
/*---------------------------------------------------------------------*/
/* Platform configuation */
 
static constexpr
int cache_line_szb = 64;
  
#if defined(TASKPARTS_X64)
static constexpr
int cpu_context_szb = 8 * 8;
#elif defined(TASKPARTS_ARM64)
static constexpr
int cpu_context_szb = 21 * 8;
#endif

using thunk = std::function<void()>;

/*---------------------------------------------------------------------*/
/* Native fork join */

using continuation_action = enum continuation_action_enum {
  continuation_initialize,
  continuation_continue,
  continuation_finish
};

auto deallocate_stack(char* stack, size_t stack_szb) -> void;

template <size_t cpu_context_szb = cpu_context_szb>
class native_continuation_family {
public:
  size_t stack_szb;
  continuation_action action;
  char gprs[cpu_context_szb];
  char* stack = nullptr;
  //  thunk f = [] { };
#ifdef TASKPARTS_USE_VALGRIND
  int valgrind_id;
#endif
  
  native_continuation_family() : action(continuation_initialize) { }
  ~native_continuation_family() {
    if (stack == nullptr) {
      return;
    }
#ifdef TASKPARTS_USE_VALGRIND
    VALGRIND_STACK_DEREGISTER(valgrind_id);
#endif
    deallocate_stack(stack, stack_szb);
    stack = nullptr;
  }
};

using native_continuation = native_continuation_family<>;

auto throw_to(native_continuation& c) -> void;
auto swap(native_continuation& current, native_continuation& next) -> void;

using outset_add_result = enum outset_add_enum {
  outset_add_success,
  outset_add_fail
};

using clone_alternative = enum cone_alternative_enum { clone_fast, clone_slow };

class native_fork_join_vertex {
public:
  alignas(cache_line_szb)
  std::atomic<int> incounter;
  alignas(cache_line_szb)
  native_fork_join_vertex* outset;
  alignas(cache_line_szb)
  native_continuation continuation;
  clone_alternative alternative;

  native_fork_join_vertex() {
    incounter.store(0, std::memory_order_relaxed);
    alternative = clone_slow;
    outset = nullptr;
  }
  virtual
  auto run() -> void = 0;
  template <typename Schedule>
  auto decrement(const Schedule& schedule) -> void {
    assert(incounter.load() > 0);
    if (alternative == clone_fast) {
      incounter.store(incounter.load(std::memory_order_relaxed) - 1, std::memory_order_relaxed);
      if (incounter.load(std::memory_order_relaxed) == 0) {
	schedule(this);
      }
      return;
    }
    if (--incounter == 0) {
      schedule(this);
    }
  }
  auto add(native_fork_join_vertex* v) -> outset_add_result {
    assert(outset == nullptr);
    outset = v;
    return outset_add_success;
  }
  template <typename Schedule>
  auto parallel_notify(const Schedule& schedule) -> void {
    if (outset == nullptr) { // e.g., the final vertex
      return;
    }
    outset->decrement(schedule);
    outset = nullptr;
  }
};

template <typename Thunk>
class native_fork_join_thunk_vertex : public native_fork_join_vertex {
public:
  Thunk& body;
  static_assert(std::is_invocable_v<Thunk&>);
  native_fork_join_thunk_vertex(Thunk& _body)
    : native_fork_join_vertex(), body(_body) { }
  auto run() -> void override {
    body();
  }
};
template <typename Thunk>
auto make_thunk_vertex(Thunk& f) -> native_fork_join_thunk_vertex<Thunk> {
  return native_fork_join_thunk_vertex(f);
}

extern
bool in_scheduler;
extern
auto __launch(native_fork_join_vertex& source) -> void;

template <typename F>
auto launch(F&& f) -> void {
  auto v = make_thunk_vertex(f);
  __launch(v);
}

class native_fork_join_scheduler_interface {
  public:
  using continuation = native_continuation;
  using vertex = native_fork_join_vertex;
  virtual
  auto self() -> vertex* = 0;
  virtual
  auto initialize_fork(vertex* parent, vertex* child1, vertex* child2) -> void  = 0;
  virtual
  auto worker_continuation() -> continuation& = 0;
  virtual
  auto pop() -> vertex* = 0;
  virtual
  auto schedule(vertex* v) -> void = 0;
};

extern
auto my_scheduler() -> native_fork_join_scheduler_interface*;    

extern "C"
void* context_save(char*);
  
template <typename F1, typename F2>
__attribute__ ((returns_twice))
auto fork2join(F1&& f1, F2&& f2) -> void {
  if (in_scheduler) {
    auto f11 = [] {};
    auto v1 = make_thunk_vertex(f11);
    auto v2 = make_thunk_vertex(f2);
    auto execute_f2 = [&]() { std::forward<F2>(f2)(); };
    native_fork_join_scheduler_interface& s = *my_scheduler();
    auto& v0 = *s.self(); // parent task
    v0.alternative = clone_slow;
    if (context_save(&(v0.continuation.gprs[0]))) {
      { assert(s.self() == &v0); assert(v0.continuation.action == continuation_continue); }
      v0.continuation.action = continuation_finish;
      return; // exit point of parent task if v2 was stolen
    }
    v0.continuation.action = continuation_continue;
    s.initialize_fork(&v0, &v1, &v2);
    v1.continuation.action = continuation_continue;
    swap(v1.continuation, s.worker_continuation()); { assert(s.self() == &v1); }
    v1.continuation.action = continuation_finish;
    std::forward<F1>(f1)(); { assert(s.self() == &v1); }
    auto vb = s.pop();
    if (vb == nullptr) { // v2 was stolen
      { assert(v1.continuation.action == continuation_finish); }
      throw_to(s.worker_continuation());
    }
    v0.alternative = clone_fast;
    // v2 was not stolen
    { assert(vb == &v2); assert(v2.incounter.load() == 0); assert(v0.incounter.load() == 2); }
    v2.continuation.action = continuation_continue;
    s.schedule(&v2);
    swap(v2.continuation, s.worker_continuation());
    v2.continuation.action = continuation_finish;
    { assert(s.self() == &v2); assert(v0.incounter.load() == 1); }
    execute_f2();
    { assert(s.self() == &v2); assert(v0.continuation.action == continuation_continue); }
    swap(v0.continuation, s.worker_continuation());
    { assert(s.self() == &v0); assert(v0.incounter.load() == 0); }
    v0.continuation.action = continuation_finish;
    // exit point of parent task if v2 was not stolen
  } else { { assert(get_my_id() == 0); } // need to launch a new scheduler instance
    in_scheduler = true;
    launch([&] { fork2join(f1, f2); });
    in_scheduler = false;
  }
}

/*---------------------------------------------------------------------*/
/* Continuations */

class minimal_continuation {
public:
  continuation_action action;
  thunk f;
  minimal_continuation() : f([] {}), action(continuation_initialize) { }
};

auto throw_to(minimal_continuation& c) -> void;
auto swap(minimal_continuation&, minimal_continuation& next) -> void;
template <typename F = thunk>
auto new_continuation(minimal_continuation& c, const F& f) -> void* {
  new (&c.f) thunk(f);
  c.action = continuation_finish;
  return nullptr;
}
auto get_action(minimal_continuation& c) -> continuation_action&;

using trampoline_block_label = int;

class trampoline_continuation {
public:
  minimal_continuation mc;
  trampoline_block_label next;
};

auto throw_to(trampoline_continuation& c) -> void;

auto swap(trampoline_continuation&, trampoline_continuation& next) -> void;

template <typename F = thunk>
auto new_continuation(trampoline_continuation& c, const F& f) -> void* {
  c.next = 0;
  return new_continuation(c.mc, f);
}

auto get_action(trampoline_continuation& c) -> continuation_action&;

auto get_trampoline(trampoline_continuation& c) -> trampoline_block_label&;

using continuation_types = enum continuation_types_enum {
  continuation_minimal,
  continuation_trampoline,
  continuation_ucontext,
  continuation_uninitialized
};

class continuation {
public:
  continuation_types continuation_type;
  continuation() : continuation_type(continuation_uninitialized) { }
  union U {
    U() {}
    ~U() {}
    minimal_continuation m;
    trampoline_continuation t;
    //    native_continuation u;
  } u;
};

auto throw_to(continuation& c) -> void;
auto swap(continuation& current, continuation& next) -> void;
template <typename F = thunk >
auto new_continuation(continuation& c, F f) -> void* {
  if (c.continuation_type == continuation_minimal) {
    return new_continuation(c.u.m, f);
  } else if (c.continuation_type == continuation_trampoline) {
    return new_continuation(c.u.t, f);
  } else {
    assert(c.continuation_type == continuation_ucontext);
    //    new_continuation(c.u.u, f);
    return nullptr;
  }
}
auto get_action(continuation& c) -> continuation_action&;
auto get_trampoline(continuation& c) -> trampoline_block_label&;
  
/*---------------------------------------------------------------------*/
/* Task DAG */

class fork_join_edges {
public:
  alignas(cache_line_szb)
  std::atomic<uint64_t> incounter;
  alignas(cache_line_szb)
  void* outset;
  
  auto new_incounter() -> void;
  auto increment() -> void;
  template <typename Schedule, typename Vertex_handle>
  auto decrement(Vertex_handle v, const Schedule& schedule) -> void {
    assert(incounter.load() > 0);
    if (--incounter == 0) {
      schedule(v);
    }
  }
  auto new_outset() -> void;
  template <typename Vertex_handle>
  auto add(Vertex_handle* outset, Vertex_handle v) -> outset_add_result {
    assert(outset != nullptr);
    assert(*outset == nullptr);
    assert(v != nullptr);
    *outset = v;
    return outset_add_success;
  }
  template <typename Decrement>
  auto parallel_notify(const Decrement& decrement) -> void {
    if (outset == nullptr) { // e.g., the final vertex
      return;
    }
    decrement(outset);
    outset = nullptr;
  }
};

template <
typename Edge_endpoints = fork_join_edges,
typename Continuation = continuation>
class vertex_family {
public:
  alignas(cache_line_szb)
  Edge_endpoints edges;
  alignas(cache_line_szb)
  Continuation continuation;
  
  virtual
  ~vertex_family() { }
  virtual
  auto run() -> void = 0;
  virtual
  auto deallocate() -> void = 0;
};

using vertex = vertex_family<fork_join_edges, continuation>;

template <typename Vertex_handle = vertex*>
auto new_incounter(Vertex_handle v) -> void {
  v->edges.new_incounter();
}

template <typename Vertex_handle = vertex*>
auto increment(Vertex_handle v) -> void {
  v->edges.increment();
}

template <typename Scheduler, typename Vertex_handle = vertex*>
auto decrement(Vertex_handle v) -> void {
  v->edges.template decrement(v, [] (Vertex_handle __v) {
    Scheduler::Schedule(__v);
  });
}

template <typename Vertex_handle = vertex*>
auto new_outset(Vertex_handle v) -> void {
  v->edges.new_outset();
}

template <typename Vertex_handle = vertex*>
auto add(Vertex_handle v, Vertex_handle u) -> outset_add_result {
  return v->edges.add((Vertex_handle*)&(v->edges.outset), u);
}

template <typename Scheduler, typename Vertex_handle = vertex*>
auto parallel_notify(Vertex_handle v) -> void {
  v->edges.parallel_notify([] (void* _v) {
    decrement<Scheduler>((Vertex_handle)_v);
  });
}

auto tick() -> void;

template <typename Local_reset, typename Global_reset>
auto reset_scheduler(const Local_reset& local_reset,
                     const Global_reset& global_reset,
                     bool global_first) -> void {
  if (get_nb_workers() == 1) {
    if (global_first) {
      global_reset();
      local_reset();
    } else {
      local_reset();
      global_reset();
    }
    return;
  }
  std::atomic<size_t> nb_workers_reset(0);
  std::atomic<bool> ready = false;
  std::function<void(size_t, size_t)> global_first_rec;
  global_first_rec = [&] (size_t lo, size_t hi) {
    if (++nb_workers_reset == get_nb_workers()) {
      global_reset();
      ready.store(true);
      local_reset();
      return;
    }
    fork2join([&] {
      while (! ready.load()) {
	tick();
      }
      local_reset();
    }, [&] {
      global_first_rec(lo + 1, hi);
    });
  };
  std::function<void(size_t, size_t)> local_first_rec;
  local_first_rec = [&] (size_t lo, size_t hi) {
    local_reset();
    if (++nb_workers_reset == get_nb_workers()) {
      global_reset();
      ready.store(true);
      return;
    }
    fork2join([&] {
      while (! ready.load()) {
	tick();
      }
    }, [&] {
      local_first_rec(lo + 1, hi);
    });
  };
  if (global_first) {
    global_first_rec(0, get_nb_workers());
  } else {
    local_first_rec(0, get_nb_workers());
  }
}

auto ping_all_workers() -> void;

auto get_benchmark_warmup_secs() -> double;
auto get_benchmark_verbose() -> bool;
auto get_benchmark_nb_repeat() -> size_t;

auto instrumentation_on_enter_work() -> void;
auto instrumentation_on_exit_work() -> void;
auto instrumentation_start() -> void;
auto instrumentation_capture() -> void;
auto instrumentation_reset() -> void;
auto instrumentation_report(std::string outfile = "") -> void;

auto teardown() -> void;

auto print_taskparts_help_message() -> void;
auto report_taskparts_configuration() -> void;
auto log_program_point(int line_nb, const char* source_fname, void* ptr) -> void;

auto launch_dag_calculus(vertex*) -> void;

template <typename F>
class dag_calculus_vertex : public vertex {
public:
  F f;
  dag_calculus_vertex(const F& f) : f(f) { }
  ~dag_calculus_vertex() { }
  auto run() -> void {
    f();
  }
  auto deallocate() -> void {
    delete this;
  }
};

auto _create_vertex(vertex* v,
		   continuation_types continuation_type) -> vertex*;

auto new_edge(vertex* src, vertex* dst) -> void;

auto release(vertex* v) -> void;

auto self() -> vertex*;

auto capture_continuation(vertex* v) -> vertex*;

template <typename F>
auto create_vertex(const F& f,
		   continuation_types continuation_type = continuation_ucontext) -> vertex* {
  return _create_vertex(new dag_calculus_vertex(f), continuation_type);
}


auto test_fib_dag_calculus() -> void;

} // namespace taskparts

#ifdef TASKPARTS_HEADER_ONLY
#include "../../src/taskparts.cpp"
#endif

#define TASKPARTS_LOG_PROGRAM_POINT(p) \
  taskparts::log_program_point(__LINE__, __FILE__, p)
