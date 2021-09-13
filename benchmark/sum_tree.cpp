#include <vector>
#include <tuple>
#include <functional>
#include <stdio.h>

#include "taskparts/chaselev.hpp"
#include "taskparts/tpalrts.hpp"

#include "sum_tree_rollforward_decls.hpp"

namespace taskparts {
template <typename Scheduler>
class fiber {
public:

  alignas(TASKPARTS_CACHE_LINE_SZB)
  std::atomic<std::size_t> incounter;

  fiber() : incounter(1) { }

  virtual
  fiber_status_type exec() {
    return fiber_status_exit_launch;
  }

  virtual
  void finish() {
    delete this;
  }

  auto is_ready() -> bool {
    return true;
  }

  auto schedule() {
    Scheduler::schedule(this);
  }

};

using tpalrts = minimal_scheduler<minimal_stats, minimal_logging, minimal_elastic,
				  ping_thread_worker, ping_thread_interrupt>;
using my_scheduler = chase_lev_work_stealing_scheduler<tpalrts, fiber,
						       minimal_stats, minimal_logging, minimal_elastic,
						       ping_thread_worker, ping_thread_interrupt>;

void initialize() {
  rollforward_table = {
    #include "sum_tree_rollforward_map.hpp"
  };
  initialize_rollfoward_table();
  initialize_machine();
}

void destroy() {
  teardown_machine();
}

} // end namespace

class task : public taskparts::fiber<taskparts::tpalrts> {
public:

  std::function<void()> f;

  task(std::function<void()> f) : fiber(), f(f) { }

  taskparts::fiber_status_type exec() {
    f();
    return taskparts::fiber_status_exit_launch;
  }
  
};

void join(void* _t) {
  task* t = (task*)_t;
  auto in = --t->incounter;
  if (in == 0) {
    t->schedule();
  }
}

void release(task* t) {
  join(t);
}

void fork(task* t, task* k) {
  k->incounter++;
  release(t);
}

int answer;

class node {
public:
  int value;
  node* left;
  node* right;

  node(int value)
    : value(value), left(nullptr), right(nullptr) { }
  node(int value, node* left, node* right)
    : value(value), left(left), right(right) { }
};

static constexpr
int nullprml = -1;

using kont_tag = enum kont_enum { K1, K2, K3, K4, K5 };

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
  vhbkont(kont_tag tag) // K3
    : tag(tag) { }
};

extern
void sum_heartbeat(node* n, std::vector<vhbkont>& k, int prmlfr, int prmlbk);

void sum_serial(node* n);

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

int main() {
  taskparts::initialize();
  
  auto ns = std::vector<node*>(
    { 
      nullptr,
      new node(123),
      new node(1, new node(2), nullptr),
      new node(1, nullptr, new node(2)),  
      new node(1, new node(200), new node(3)), 
      new node(1, new node(2), new node(3, new node(4), new node(5))),
      new node(1, new node(3, new node(4), new node(5)), new node(2)),
    });
  for (auto n : ns) {
    answer = -1;
    
    auto f_body = new task([&] {
      std::vector<vhbkont> k({vhbkont(K3)});
      sum_heartbeat(n, k, nullprml, nullprml);
    });
    join(f_body);
    taskparts::my_scheduler::launch();
    taskparts::destroy();

    //sum_serial(n);
    printf("answer=%d\n",answer);
  }
  return 0;
}
