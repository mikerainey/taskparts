#include <cmath>
#include <atomic>
#include <deque>
#include <iostream>
#include <cstdlib>
#include <chrono>
#include <taskparts/taskparts.hpp>
#include "fib_serial.hpp"

inline
uint64_t hash(uint64_t u) {
  uint64_t v = u * 3935559000370003845ul + 2691343689449507681ul;
  v ^= v >> 21;
  v ^= v << 37;
  v ^= v >>  4;
  v *= 4768777513237032717ul;
  v ^= v << 20;
  v ^= v >> 41;
  v ^= v <<  5;
  return v;
}

namespace taskparts {

auto test_bintree() -> void {
  std::atomic<uint64_t> count(0);
  std::function<void(uint64_t, uint64_t)> bintree_rec;
  bintree_rec = [&] (uint64_t lo, uint64_t hi) -> void {
    if ((lo + 1) == hi) {
      count++;
      return;
    }
    auto mid = (lo + hi) / 2;
    fork2join([&] { bintree_rec(lo, mid); }, [&] { bintree_rec(mid, hi); });
  };
  uint64_t n = 1<<20;
  bintree_rec(0, n);
  assert(count.load() == n);
}

auto test_fib_native() -> void {
  std::function<uint64_t(uint64_t)> fib_rec;
  fib_rec = [&] (uint64_t n) -> uint64_t {
    if (n < 2) {
      return n;
    }
    uint64_t rs[2];
    fork2join([&] { rs[0] = fib_rec(n - 1); }, [&] { rs[1] = fib_rec(n - 2); });
    return rs[0] + rs[1];
  };
  uint64_t n = 30;
  uint64_t dst = fib_rec(n);
  assert(fib_serial(n) == dst);
}
  
template <typename F, typename R, typename V>
auto reduce(const F& f, const R& r, V z, size_t lo, size_t hi) -> V {
  if (lo == hi) {
    return z;
  }
  if ((lo + 1) == hi) {
    return r(f(lo), z);
  }
  V rs[2];
  auto mid = (lo + hi) / 2;
  fork2join([&] { rs[0] = reduce(f, r, z, lo, mid); },
            [&] { rs[1] = reduce(f, r, z, mid, hi); });
  return r(rs[0], rs[1]);
}

template <typename F>
auto test_variable_workload(const F& f) {
  auto max_items = 1<<10;
  size_t i = 0;
  uint64_t r = 1234;
  fork2join([&] {
    while (f()) {
      auto n = (size_t)((sin(i++) * max_items / 2) + max_items / 2);
      assert(n < max_items);
      //    aprintf("n=%d\n",n);
      r = reduce([&] (uint64_t i) { return hash(i); },
                 [&] (uint64_t r1, uint64_t r2) { return hash(r1 | r2); },
                 r, 0, n);
    }
  }, [] {});
  
  printf("i = %lu, r = %lu\n", i, r);
}

auto test_variable_workload() {
  auto secs = 4.0;
  auto s = now();
  test_variable_workload([&] {
    return since(s) < secs;
  });
}

auto test_reset_scheduler() -> void {
  auto t = [] {
    { // global first
      std::atomic<size_t> nb_workers(0);
      std::atomic<bool> global(false);
      reset_scheduler([&] {
        assert(global.load());
        nb_workers++;
      }, [&] {
        assert(nb_workers.load() == 0);
        global.store(true);
      }, true);
      assert(nb_workers.load() == get_nb_workers());
    }
    { // local first
      std::atomic<size_t> nb_workers(0);
      std::atomic<bool> global(false);
      reset_scheduler([&] {
        assert(! global.load());
        nb_workers++;
      }, [&] {
        assert(nb_workers.load() == get_nb_workers());
        global.store(true);
      }, false);
      assert(global.load());
    }
  };
  for (size_t i = 0; i < 30; i++) {
    t();
  }
}


/*---------------------------------------------------------------------*/
/* Control-flow graph */

template <int nb_labels, typename Frame>
class cfg_definition {
public:
  // basic blocks
  std::vector<std::function<trampoline_block_label(Frame&)>> blocks;
  // returns true if given label is that of an exit block
  std::function<bool(trampoline_block_label)> is_exit_block;
  
  cfg_definition() :
  is_exit_block([] (trampoline_block_label l) {
    return l == nb_labels; // defaultly, the last block is the one and only exit block
  }) {
    blocks.resize(nb_labels);
  }
};

template <int nb_labels, typename Frame>
class cfg_vertex : public vertex {
public:
  
  static
  cfg_definition<nb_labels, Frame> cfg;
  
  alignas(cache_line_szb)
  Frame frame;
  
  cfg_vertex() : vertex() {
    continuation.continuation_type = continuation_trampoline;
    continuation.u.t.next = 0; // the first block is the one and only entrance block
  }
  auto run() -> void {
    auto& next = continuation.u.t.next;
    assert(next >= 0);
    assert(next <= cfg.blocks.size());
    if (cfg.is_exit_block(next)) {
      get_action(continuation) = continuation_finish;
      return;
    }
    get_action(continuation) = continuation_continue;
    next = cfg.blocks[next](frame);
  }
  auto deallocate() -> void {
    delete this;
  }
};

template <int nb_labels, typename Frame>
cfg_definition<nb_labels, Frame> cfg_vertex<nb_labels, Frame>::cfg;

/*---------------------------------------------------------------------*/
/* DAG calculus scheduler */

// TODOs:
//   - refactor serial_scheduler so that it can be instantiated into DFS, BFS, pseudo-random schedules
//   - can native fork join be implemented using C++ coroutines and on demand spawning of threads?
//   - fix thunk issue in ucontext by moving the thunk into the ucontext class
//   - fix space leak in the stack used by ucontext
//   - find a better way of passing continuation-action signals in ucontext, which currently has to signal the termination of a task by performing updates at exit points
//   - randomized dag generators
//   - add proper implementation for DAG calculus outset

template <typename Vertex_handle = vertex*>
class dag_calculus_scheduler {
public:
  virtual
  auto schedule(Vertex_handle t) -> void = 0;
  virtual
  auto create_task(thunk f,
                   continuation_types continuation_type = continuation_ucontext) -> Vertex_handle = 0;
  virtual
  auto release(Vertex_handle v) -> void = 0;
  virtual
  auto new_edge(Vertex_handle v, Vertex_handle u) -> void = 0;
  virtual
  auto yield() -> void = 0;
  virtual
  auto self() -> Vertex_handle = 0;
  virtual
  auto launch(Vertex_handle root_task,
              continuation_types continuation_type = continuation_ucontext) -> void = 0;
};

template <typename Vertex_handle>
using dag_calculus_scheduler_stack = std::deque<dag_calculus_scheduler<Vertex_handle>*>;

dag_calculus_scheduler_stack<vertex*> schedulers;

/*---------------------------------------------------------------------*/
/* Serial scheduler */

auto my_scheduler() -> dag_calculus_scheduler<>*;

class serial_dag_calculus_scheduler : public dag_calculus_scheduler<> {
public:
  class thunk_vertex : public vertex {
  public:
    alignas(cache_line_szb)
    thunk body;
    
    thunk_vertex(thunk body) : vertex(), body(body) { }
    auto run() -> void {
      body();
    }
    auto deallocate() -> void {
      delete this;
    }
  };

  std::deque<vertex*> ready;
  vertex* current = nullptr;
  continuation worker_continuation;
  
  auto schedule(vertex* v) -> void {
    assert(v->edges.incounter.load() == 0);
    ready.push_back(v);
  }
  auto enter() -> void {
    current->run();
    throw_to(worker_continuation);
  }
  auto loop() -> void {
    while (! ready.empty()) {
      current = ready.back();
      assert(current != nullptr);
      ready.pop_back();
      increment(current);
      if (get_action(current->continuation) == continuation_initialize) {
        new_continuation(current->continuation, [] { ((serial_dag_calculus_scheduler*)my_scheduler())->enter(); });
      }
      swap(worker_continuation, current->continuation);
      if (get_action(current->continuation) == continuation_finish) {
        parallel_notify<serial_dag_calculus_scheduler>(current);
        current->deallocate();
      } else {
        assert(get_action(current->continuation) == continuation_continue);
        decrement<serial_dag_calculus_scheduler>(current);
      }
    }
  }
  auto create_task(thunk f,
                   continuation_types continuation_type = continuation_ucontext) -> vertex* {
    auto v = new thunk_vertex(f);
    v->continuation.continuation_type = continuation_type;
    initialize_vertex(v);
    return v;
  }
  auto release(vertex* v) -> void {
    decrement<serial_dag_calculus_scheduler>(v);
  }
  auto new_edge(vertex* v, vertex* u) -> void {
    increment(u);
    if (add(v, u) == outset_add_fail) {
      decrement<serial_dag_calculus_scheduler>(u);
    }
  }
  auto yield() -> void {
    get_action(current->continuation) = continuation_continue;
    swap(current->continuation, worker_continuation);
  }
  auto self() -> vertex* {
    return current;
  }
  auto launch(vertex* root_task,
              continuation_types continuation_type = continuation_ucontext) -> void {
    schedulers.push_back(this);
    worker_continuation.continuation_type = continuation_type;
    new_continuation(worker_continuation, [] {});
    initialize_vertex(root_task);
    release(root_task);
    loop();
    schedulers.pop_back();
  }
  static
  auto initialize_vertex(vertex* v) -> void {
    new_incounter(v);
    new_outset(v);
    increment(v);
  }
  static
  auto Schedule(vertex* v) -> void {
    my_scheduler()->schedule(v);
  }
};

auto launch_rec(dag_calculus_scheduler_stack<vertex*>& stack,
                size_t position,
                dag_calculus_scheduler<vertex*>* target,
                thunk f) -> void {
  assert(position < stack.size());
  auto s = stack[position];
  if (s == target) {
    s->launch(s->create_task(f));
  } else {
    s->launch(s->create_task([&] {
      launch_rec(stack, position + 1, target, f);
    }));
  }
}

/*---------------------------------------------------------------------*/
/* Scheduler interface */

auto my_scheduler() -> dag_calculus_scheduler<vertex*>* {
  assert(! schedulers.empty());
  return schedulers.back();
}

auto launch(thunk f, continuation_types continuation_type = continuation_ucontext) -> void {
  serial_dag_calculus_scheduler s;
  auto root_task = s.create_task(f, continuation_type);
  s.launch(root_task, continuation_type);
}

auto create_task(thunk f,
                 continuation_types continuation_type = continuation_ucontext) -> vertex* {
  return my_scheduler()->create_task(f, continuation_type);
}

auto release(vertex* v) -> void {
  my_scheduler()->release(v);
}
auto self() -> vertex*;
auto new_edge(vertex* v, vertex* u = self()) -> void {
  my_scheduler()->new_edge(v, u);
}
auto yield() -> void {
  my_scheduler()->yield();
}
auto self() -> vertex* {
  return my_scheduler()->self();
}
auto launch(vertex* root_task,
            continuation_types continuation_type = continuation_ucontext) -> void {
  my_scheduler()->launch(root_task, continuation_type);
}

auto my_trampoline() -> trampoline_block_label& {
  return get_trampoline(my_scheduler()->self()->continuation);
}

auto my_continuation_action() -> continuation_action& {
  return get_action(my_scheduler()->self()->continuation);
}
  
} // end namespace

/*---------------------------------------------------------------------*/
/* Example programs */

using namespace taskparts;

// Minimal Fibonacci function

template <typename Scheduler = serial_dag_calculus_scheduler>
auto fib_minimal(uint64_t n, uint64_t* dst) -> void {
  if (n < 2) {
    *dst = n;
    return;
  }
  auto dst2 = new uint64_t[2];
  auto vj = create_task([=] {
    *dst = dst2[0] + dst2[1];
    delete [] dst2;
  }, continuation_minimal);
  { // capture the current "task-level continuation"
    // and transfer it the new join continuation
    auto& vk = self()->edges.outset;
    vj->edges.outset = vk;
    vk = nullptr;
  }
  vertex* vs[2];
  for (int i = 1; i >= 0; i--) {
    vs[i] = create_task([=] {
      fib_minimal(n - (i + 1), &dst2[i]);
    }, continuation_minimal);
  }
  for (auto v : vs) {
    new_edge(v, vj);
  }
  for (auto v : vs) {
    release(v);
  }
  release(vj);
}

// CFG-based Fibonacci function

using fib_trampoline = enum fib_trampoline_enum : int { fib_entry=0, fib_combine, fib_exit, fib_nb_blocks = fib_exit };
using fib_frame = struct fib_struct { uint64_t n; uint64_t* dst; uint64_t* dst2; };
class fib_cfg : public cfg_vertex<fib_nb_blocks, fib_frame> {
public:
  fib_cfg() : cfg_vertex() { }
};
auto define_fib_cfg() -> void {
  auto& blocks = fib_cfg::cfg.blocks;
  assert(fib_nb_blocks == blocks.size());
  blocks[fib_entry] = std::function<fib_trampoline(fib_frame&)>([] (fib_frame& f) {
    if (f.n < 2) {
      *f.dst = f.n;
      return fib_exit;
    }
    f.dst2 = new uint64_t[2];
    vertex* vs[2];
    for (int i = 1; i >= 0; i--) {
      auto vi = new fib_cfg;
      vi->frame.n = f.n - (i + 1);
      vi->frame.dst = &f.dst2[i];
      serial_dag_calculus_scheduler::initialize_vertex(vi);
      vs[i] = vi;
    }
    for (auto v : vs) {
      new_edge(v);
    }
    for (auto v : vs) {
      release(v);
    }
    return fib_combine;
  });
  blocks[fib_combine] = std::function<fib_trampoline(fib_frame&)>([] (fib_frame& f) {
    *f.dst = f.dst2[0] + f.dst2[1];
    delete [] f.dst2;
    return fib_exit;
  });
}

// Microcontext-based Fibonacci function

auto fib_ucontext(uint64_t n) -> uint64_t {
  if (n < 2) {
    my_continuation_action() = continuation_finish;
    return n;
  }
  uint64_t dst2[2];
  vertex* vs[2];
  for (int i = 1; i >= 0; i--) {
    vs[i] = create_task([=, &dst2] {
      dst2[i] = fib_ucontext(n - (i + 1));
    });
  }
  for (auto v : vs) {
    new_edge(v);
  }
  for (auto v : vs) {
    release(v);
  }
  yield();
  my_continuation_action() = continuation_finish;
  return dst2[0] + dst2[1];
}

auto test_nested_scheduler() -> void {
  int i = 0;
  serial_dag_calculus_scheduler s1;
  auto root_task1 = s1.create_task([&] {
    serial_dag_calculus_scheduler s2;
    i++;
    auto root_task2 = s2.create_task([&] {
      i++;
    }, continuation_minimal);
    s2.launch(root_task2, continuation_minimal);
    i++;
  }, continuation_minimal);
  s1.launch(root_task1, continuation_minimal);
  assert(i == 3);
}

/*---------------------------------------------------------------------*/
/* Driver */

class unit_tests {
public:
  std::vector<thunk> tests;
  auto add(thunk f) -> void {
    tests.push_back(f);
  }
  auto run() -> void {
#ifndef TASKPARTS_RUN_UNIT_TESTS
    return;
#endif
    for (auto& f : tests) {
      f();
    }
  }
};

unit_tests tests;

int main() { /*
  fork2join([&] { }, [&] { aprintf("fib=%lu\n",fib_serial(35)); });
  return 0; */
  test_fib_dag_calculus();
  return 0;
  test_variable_workload();
  //return 0;
  tests.add([] { test_fib_native(); });
  tests.add([] { test_bintree(); });
  tests.add([] { test_reset_scheduler(); });
  tests.add([] { // control-flow graph / dag calculus
    uint64_t dst = 123;
    uint64_t n = 8;
    define_fib_cfg();
    serial_dag_calculus_scheduler s;
    schedulers.push_back(&s);
    auto root_task = new fib_cfg;
    root_task->frame.n = n;
    root_task->frame.dst = &dst;
    serial_dag_calculus_scheduler::initialize_vertex(root_task);
    s.launch(root_task, continuation_trampoline);
    schedulers.pop_back();
    assert(fib_serial(n) == dst);
  });
  tests.add([] { // basic dag calculus
    uint64_t dst = 123;
    uint64_t n = 8;
    serial_dag_calculus_scheduler s;
    schedulers.push_back(&s);
    auto root_task = s.create_task([&] {
      fib_minimal(n, &dst);
    }, continuation_minimal);
    s.launch(root_task, continuation_minimal);
    schedulers.pop_back();
    assert(fib_serial(n) == dst);
  });
  tests.add([] { test_nested_scheduler(); });
  /*
  tests.add([] { // native continuation
    uint64_t dst = 123;
    uint64_t n = 8;
    launch([&] {
      dst = fib_ucontext(n);
    });
    assert(fib_serial(n) == dst);
  }); */

  tests.run();

  return 0;
}
