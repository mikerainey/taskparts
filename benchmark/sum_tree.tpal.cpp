#ifndef TASKPARTS_TPALRTS
#error "need to compile with tpal flags, e.g., TASKPARTS_TPALRTS"
#endif
#include <deque>
#include <tuple>
#include <functional>
#include <stdio.h>

#include "sum_tree.hpp"
#include "sum_tree_rollforward_decls.hpp"
#include "sum_tree.tpal.hpp"

auto tpalrts_prmlist_pop_front(tpalrts_prml prml) -> tpalrts_prml {
  auto prev = prml.front;
  auto next = prev->next;
  if (next == nullptr) {
    prml.back = nullptr;
  } else {
    next->prev = nullptr;
    prev->next = nullptr;
  }
  prml.front = next;
  return prml;
}

class task : public taskparts::fiber<taskparts::bench_scheduler> {
public:

  std::function<void()> f;

  task(std::function<void()> f) : fiber(), f(f) { }

  taskparts::fiber_status_type run() {
    f();
    return taskparts::fiber_status_finish;
  }

  void finish() {
    delete this;
  }

  void incr_incounter() {
    fiber::incounter++;
  }
  
};

void release(task* t) {
  t->release();
}

void join(task* t) {
  release((task*)t);
}

void fork(task* t, task* k) {
  k->incr_incounter();
  release(t);
}

extern
void sum_heartbeat(node* n, std::deque<vhbkont>& k, tpalrts_prml prml);

auto enclosing_frame_pointer_of(tpalrts_prml_node* n) -> vhbkont* {
  auto r = vhbkont(K1, nullptr, (tpalrts_prml_node*)nullptr);
  auto d = (char*)(&r.u.k1.prml_node)-(char*)(&r);
  auto p = (char*)n;
  auto h = p - d;
  return (vhbkont*)h;
}

tpalrts_prml sum_tree_heartbeat_handler(tpalrts_prml prml) {
  if (prml.front == nullptr) {
    return prml;
  }
  auto f_fr = enclosing_frame_pointer_of(prml.front);
  assert(f_fr->tag == K1);
  auto n = f_fr->u.k1.n;
  auto s = new int[2];
  auto kp = new std::deque<vhbkont>;
  auto tj = new task([=] {
    std::deque<vhbkont> kj;
    kp->swap(kj);
    *f_fr = vhbkont(K2, s[0] + s[1], n);
    delete kp;
    delete [] s;	    
    sum_heartbeat(nullptr, kj, tpalrts_prml());
  });
  fork(new task([=] {
    std::deque<vhbkont> k2({vhbkont(K5, s, tj)});
    sum_heartbeat(n->right, k2, tpalrts_prml());
  }), tj);
  prml = tpalrts_prmlist_pop_front(prml);
  *f_fr = vhbkont(K4, s, tj, kp);
  return prml;
}

namespace taskparts {
auto initialize_rollforward() {
  rollforward_table = {
    #include "sum_tree_rollforward_map.hpp"
  };
  initialize_rollfoward_table();
}
} // end namespace

int main() {
  taskparts::initialize_rollforward();

  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    taskparts::nativefj_fiber<decltype(sched)>::fork1join(new task([&] {
      auto _f = taskparts::nativefj_fiber<decltype(sched)>::current_fiber.mine();
      std::deque<vhbkont> k({vhbkont(K3, new task([=] {
	_f->release();
      }))});
      sum_heartbeat(n0, k, tpalrts_prml());
    }));
  }, [&] (auto sched) { gen_input(sched); }, [&] (auto sched) { teardown(); });
  
  printf("answer=%d\n", answer);
  
  return 0;
}
