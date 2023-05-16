#pragma once

#include <functional>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
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

namespace taskparts {

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
/* Continuations */

using continuation_action = enum continuation_action_enum {
  continuation_initialize,
  continuation_continue,
  continuation_finish
};

template <size_t cpu_context_szb = cpu_context_szb>
class native_continuation_family {
public:
  continuation_action action;
  char gprs[cpu_context_szb];
  char* stack = nullptr;
  thunk f = [] { };
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
    std::free(stack);
  }
};

using native_continuation = native_continuation_family<>;

auto new_continuation(native_continuation& c, thunk f) -> void*;
auto throw_to(native_continuation& c) -> void;
auto swap(native_continuation& current, native_continuation& next) -> void;
auto get_action(native_continuation& c) -> continuation_action&;

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
  continuation_ucontext
};

class continuation {
public:
  continuation_types continuation_type;
  continuation() : continuation_type(continuation_ucontext) {}
  union U {
    U() {}
    ~U() {}
    minimal_continuation m;
    trampoline_continuation t;
    native_continuation u;
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
    return new_continuation(c.u.u, f);
  }
}
auto get_action(continuation& c) -> continuation_action&;
auto get_trampoline(continuation& c) -> trampoline_block_label&;
  
/*---------------------------------------------------------------------*/
/* Native fork join */

using outset_add_result = enum outset_add_enum {
  outset_add_success,
  outset_add_fail
};

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

class continuation;

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

using native_fork_join_continuation = native_continuation;
using native_fork_join_vertex = vertex_family<fork_join_edges, native_fork_join_continuation>;
template <typename Thunk>
class native_fork_join_thunk_vertex : public native_fork_join_vertex {
public:
  alignas(cache_line_szb)
  Thunk body;

  native_fork_join_thunk_vertex(const Thunk& body) : native_fork_join_vertex(), body(body) { }
  auto run() -> void {
    body();
  }
  auto deallocate() -> void { } // does nothing b/c we assume stack allocation
};

extern
bool in_scheduler;
extern
auto __launch(native_fork_join_vertex& source) -> void;
extern
auto __fork2join(native_fork_join_vertex& v1, native_fork_join_vertex& v2) -> void;

template <typename F>
auto launch(const F& f) -> void {
  native_fork_join_thunk_vertex<F> v(f);
  __launch(v);
}

template <typename F1, typename F2>
auto _fork2join(const F1& f1, const F2& f2) -> void {
  native_fork_join_thunk_vertex<F1> v1(f1);
  native_fork_join_thunk_vertex<F2> v2(f2);
  __fork2join(v1, v2);
}

template <typename F1, typename F2>
auto fork2join(const F1& f1, const F2& f2) -> void {
  if (in_scheduler) {
    _fork2join(f1, f2);
  } else { { assert(get_my_id() == 0); } // need to launch a new scheduler instance
    in_scheduler = true;   
    launch([&] { fork2join(f1, f2); });
    in_scheduler = false;
  }
}

/*---------------------------------------------------------------------*/
/* Task DAG */

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
/* Benchmarking */

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
  
auto default_benchmark_thunk = [] { };

auto get_benchmark_warmup_secs() -> double;
auto get_benchmark_verbose() -> bool;
auto get_benchmark_nb_repeat() -> size_t;

auto instrumentation_on_enter_work() -> void;
auto instrumentation_on_exit_work() -> void;
auto instrumentation_start() -> void;
auto instrumentation_capture() -> void;
auto instrumentation_reset() -> void;
auto instrumentation_report() -> void;

auto teardown() -> void;

template <
typename Benchmark,
typename Setup = decltype(default_benchmark_thunk),
typename Teardown = decltype(default_benchmark_thunk),
typename Reset = decltype(default_benchmark_thunk)>
auto benchmark(const Benchmark& benchmark,
               const Setup& setup = default_benchmark_thunk,
               const Teardown& teardown = default_benchmark_thunk,
               const Reset& reset = default_benchmark_thunk) -> void {
  auto warmup = [&] {
    if (get_benchmark_warmup_secs() <= 0.0) {
      return;
    }
    if (get_benchmark_verbose()) printf("======== WARMUP ========\n");
    auto warmup_start = now();
    while (since(warmup_start) < get_benchmark_warmup_secs()) {
      auto st = now();
      benchmark();
      if (get_benchmark_verbose()) printf("warmup_run %.3f\n", since(st));
      reset();
    }
    if (get_benchmark_verbose()) printf ("======== END WARMUP ========\n");
  };
  setup();
  warmup();
  for (size_t i = 0; i < get_benchmark_nb_repeat(); i++) {
    reset_scheduler([&] { // worker local
      instrumentation_on_enter_work();
    }, [&] { // global
      instrumentation_reset();
      instrumentation_start();
    }, false);
    benchmark();
    reset_scheduler([&] { // worker local
      instrumentation_on_exit_work();
    }, [&] { // global
      instrumentation_capture();
      if ((i + 1) < get_benchmark_nb_repeat()) {
        reset();
      }
      instrumentation_start();
    }, true);
  }
  instrumentation_report();
  teardown();
}

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
  }
};

template <typename F>
auto launch_dag_calculus1(const F& f) -> void {
  launch_dag_calculus(new dag_calculus_vertex<F>(f));
}
  
} // namespace taskparts

#define TASKPARTS_LOG_PROGRAM_POINT(p) \
  taskparts::log_program_point(__LINE__, __FILE__, p)
