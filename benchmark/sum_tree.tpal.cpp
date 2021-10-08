#ifndef TASKPARTS_TPALRTS
#error "need to compile with tpal flags, e.g., TASKPARTS_TPALRTS"
#endif
#include <vector>
#include <tuple>
#include <functional>
#include <stdio.h>

#include "sum_tree.hpp"
#include "sum_tree_rollforward_decls.hpp"

// todo: replace use of std::vector for stack by some faster structure, e.g., std::deque or chunkedseq

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

void join(void* t) {
  release((task*)t);
}

void fork(task* t, task* k) {
  k->incr_incounter();
  release(t);
}

class vhbkont {
public:
  kont_tag tag;
  union {
    struct {
      node* n;
      int prev;
      int next;
    } k1;
    struct {
      int s0;
      node* n;
    } k2;
    struct {
      task* tk;
    } k3;
    struct {
      int* s;
      task* tj;
      std::vector<vhbkont>* kp;
    } k4ork5;
  } u;

  vhbkont(kont_tag tag, node* n, int prev) // K1
    : tag(tag) { u.k1.n = n; u.k1.prev = prev; u.k1.next = -1; }
  vhbkont(kont_tag tag, int s0, node* n) // K2
    : tag(tag) { u.k2.s0 = s0; u.k2.n = n; }
  vhbkont(kont_tag tag, int* s, task* tj, std::vector<vhbkont>* kp) // K4
    : tag(tag) { u.k4ork5.s = s; u.k4ork5.tj = tj; u.k4ork5.kp = kp; }
  vhbkont(kont_tag tag, int* s, task* tj) // K5
    : tag(tag) { u.k4ork5.s = s; u.k4ork5.tj = tj; u.k4ork5.kp = nullptr; }
  vhbkont(kont_tag tag, task* tk) // K3
    : tag(tag) { u.k3.tk = tk; }
};

extern
void sum_heartbeat(node* n, std::vector<vhbkont>& k, int prmlfr, int prmlbk);

static constexpr
int nullprml = -1;

auto prmlist_pop_front(std::vector<vhbkont>& k, int prmlfr, int prmlbk) -> std::pair<int, int> {
  int prev = prmlfr;
  int next = k[prev].u.k1.next;
  if (next == nullprml) {
    prmlbk = nullprml;
  } else {
    k[next].u.k1.prev = nullprml;
    k[prev].u.k1.next = nullprml;
  }
  prmlfr = next;
  return std::make_pair(prmlfr, prmlbk);    
}

std::pair<int, int> sum_tree_heartbeat_handler(std::vector<vhbkont>& k, int prmlfr, int prmlbk) {
  if (prmlfr == nullprml) {
    return std::make_pair(prmlfr, prmlbk);
  }
  assert(k[prmlfr].tag == K1);
  auto n = k[prmlfr].u.k1.n;
  auto s = new int[2];
  auto kp = new std::vector<vhbkont>;
  auto tj = new task([=] {
    std::vector<vhbkont> kj;
    kp->swap(kj);
    kj[prmlfr] = vhbkont(K2, s[0] + s[1], n);
    delete kp;
    delete [] s;	    
    sum_heartbeat(nullptr, kj, nullprml, nullprml);
  });
  fork(new task([=] {
    std::vector<vhbkont> k2({vhbkont(K5, s, tj)});
    sum_heartbeat(n->right, k2, nullprml, nullprml);
  }), tj);
  auto fr = prmlfr;
  std::tie(prmlfr, prmlbk) = prmlist_pop_front(k, prmlfr, prmlbk);
  k[fr] = vhbkont(K4, s, tj, kp);
  return std::make_pair(prmlfr, prmlbk);
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
      std::vector<vhbkont> k({vhbkont(K3, new task([=] {
	_f->release();
      }))});
      sum_heartbeat(n0, k, nullprml, nullprml);
    }));
  }, [&] (auto sched) { gen_input(sched); }, [&] (auto sched) { teardown(); });
  
  printf("answer=%d\n", answer);
  
  return 0;
}
