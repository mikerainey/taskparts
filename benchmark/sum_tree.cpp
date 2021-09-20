#include <vector>
#include <tuple>
#include <functional>
#include <stdio.h>

#include "taskparts/benchmark.hpp"
#include "sum_tree_rollforward_decls.hpp"

namespace taskparts {
using scheduler = minimal_scheduler<bench_stats, bench_logging, bench_elastic, bench_worker, bench_interrupt>;

auto initialize_rollforward() {
  rollforward_table = {
    #include "sum_tree_rollforward_map.hpp"
  };
  initialize_rollfoward_table();
}
} // end namespace

class task : public taskparts::fiber<taskparts::scheduler> {
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

int answer;

class node {
public:
  int value;
  node* left;
  node* right;

  node()
    : value(1), left(nullptr), right(nullptr) { }
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

auto nb_nodes_of_height(size_t h) {
  return (1 << (h + 1)) - 1;
};

auto fill(node* tree, size_t h, size_t i) -> void {
  if (h <= 0) {
    return;
  }
  node* n = &tree[i];
  size_t i_l = i + 1;
  size_t i_r = i_l + nb_nodes_of_height(h - 1);
  n->left = &tree[i_l];
  n->right = &tree[i_r];
  fill(tree, h - 1, i_l);
  fill(tree, h - 1, i_r);
};

auto gen_perfect_tree(size_t height) -> node* {
  size_t nb_nodes = nb_nodes_of_height(height);
  node* tree = new node[nb_nodes];
  fill(tree, height, 0);
  return tree;
}

void sum_serial(node* n);

int main() {
  taskparts::initialize_rollforward();

  auto n0 = gen_perfect_tree(28);

  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    taskparts::nativefj_fiber<decltype(sched)>::fork1join(new task([&] {
      auto _f = taskparts::nativefj_fiber<decltype(sched)>::current_fiber.mine();
      std::vector<vhbkont> k({vhbkont(K3, new task([=] {
	_f->release();
      }))});
      sum_heartbeat(n0, k, nullprml, nullprml);
      //sum_serial(n0);
    }));
  });
  
  printf("answer=%d\n", answer);
  
  delete [] n0;
    
    /*
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

    taskparts::benchmark_nativeforkjoin([&] (auto sched) {
      taskparts::nativefj_fiber<decltype(sched)>::fork1join(new task([&] {
	std::vector<vhbkont> k({vhbkont(K3)});
	//sum_heartbeat(n, k, nullprml, nullprml);
	sum_serial(n);
      }));
    });

    printf("answer=%d\n", answer);
  }
    */
  
  return 0;
}
