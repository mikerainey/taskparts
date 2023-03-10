#include "taskparts/nativeforkjoin.hpp"
#ifndef TASKPARTS_TPALRTS
#error "need to compile with tpal flags, e.g., TASKPARTS_TPALRTS"
#endif
#include <taskparts/benchmark.hpp>
#include "sum_tree.hpp"

int answer;

volatile
bool heartbeat;

auto nb_nodes_of_height(size_t h) {
  return (1 << (h + 1)) - 1;
};

template <typename Scheduler=taskparts::minimal_scheduler<>>
auto fill(node* tree, size_t h, size_t i, Scheduler sched=Scheduler()) -> void {
  if (h <= 0) {
    return;
  }
  node* n = &tree[i];
  size_t i_l = i + 1;
  size_t i_r = i_l + nb_nodes_of_height(h - 1);
  n->left = &tree[i_l];
  n->right = &tree[i_r];
  taskparts::spguard([&] { return nb_nodes_of_height(h); }, [&] {
    taskparts::ogfork2join([&] {
      fill(tree, h - 1, i_l, sched);
    }, [&] {
      fill(tree, h - 1, i_r, sched);
    }, sched);
  });
};

node* n0 = nullptr;
node* n1 = nullptr;

template <typename Scheduler=taskparts::minimal_scheduler<>>
auto gen_perfect_tree(size_t height, Scheduler sched=Scheduler()) -> node* {
  size_t nb_nodes = nb_nodes_of_height(height);
  node* tree = new node[nb_nodes];
  fill(tree, height, 0, sched);
  return tree;
}

template <typename Scheduler=taskparts::minimal_scheduler<>>
auto gen_imperfect_tree(size_t height, size_t nb_spine_nodes, Scheduler sched=Scheduler()) -> std::pair<node*, node*> {
  auto pt = gen_perfect_tree(height, sched);
  size_t nb_nodes = nb_nodes_of_height(height);
  node* spine_tree = new node[nb_spine_nodes];
  n1 = spine_tree;
  taskparts::parallel_for(0, nb_spine_nodes, [&] (size_t i) {
    if ((i + 1) == nb_spine_nodes) {
      return;
    }
    spine_tree[i].left = &spine_tree[i + 1];
  }, taskparts::dflt_parallel_for_cost_fn, sched);
  auto leaf = &spine_tree[nb_spine_nodes - 1];
  size_t nb_imperfections = taskparts::cmdline::parse_or_default_long("nb_imperfections", 5);;
  for (size_t i = 0; i < nb_imperfections; i++) {
    node* s = pt;
    size_t j = 0;
    while (true) {
      int dir = taskparts::hash(i + j) % 2;
      node** s2 = (dir == 0) ? &(s->left) : &(s->right);
      if ((*s2) == nullptr) {
	*s2 = spine_tree;
	break;
      }
      s = *s2;
      j++;
    }
  }
  return std::make_pair(pt, leaf);
}

template <typename Scheduler=taskparts::minimal_scheduler<>>
auto gen_alternating_tree(size_t height, size_t nb_spine_nodes, size_t nb_alternations, Scheduler sched=Scheduler()) -> node* {
  auto p = gen_imperfect_tree(height, nb_spine_nodes, sched);
  auto n0 = p.first;
  auto l0 = p.second;
  for (size_t i = 0; i < nb_alternations; i++) {
    auto _p = gen_imperfect_tree(height, nb_spine_nodes, sched);
    l0->right = _p.first;
    l0 = _p.second;
  }
  return n0;
}

template<typename Sched>
auto gen_input(Sched sched) {
  int h = 0;
  size_t nb_spine_nodes = taskparts::cmdline::parse_or_default_long("nb_spine_nodes", 5000000);
  taskparts::cmdline::dispatcher d;
  d.add("perfect", [&] {
    h = taskparts::cmdline::parse_or_default_int("height", 28);
    n0 = gen_perfect_tree(h, sched);
  });
  d.add("imperfect", [&] {
    h = taskparts::cmdline::parse_or_default_int("height", 27);
    auto _p = gen_imperfect_tree(h, nb_spine_nodes, sched);
    n0 = _p.first;
  });
  d.add("alternating", [&] {
    h = taskparts::cmdline::parse_or_default_int("height", 23);
    size_t nb_alternations = taskparts::cmdline::parse_or_default_int("alternations", 2);
    n0 = gen_alternating_tree(h, nb_spine_nodes, nb_alternations, sched);
  });
  d.dispatch_or_default("input", "perfect");
}

auto teardown() {
  delete [] n0;
  if (n1 != nullptr) {
    delete [] n1;
  }
}


auto tpalrts_prmlist_push_back(tpalrts_prml prml, tpalrts_prml_node* t) -> tpalrts_prml {
  if (prml.back != nullptr) {
    prml.back->next = t;
  }
  prml.back = t;
  if (prml.front == nullptr) {
    prml.front = prml.back;
  }
  return prml;
}

auto tpalrts_prmlist_pop_back(tpalrts_prml prml) -> tpalrts_prml {
  auto next = prml.back;
  auto prev = next->prev;
  if (prev == nullptr) {
    prml.front = nullptr;
  } else {
    prev->next = nullptr;
    next->prev = nullptr;
  }
  prml.back = prev;
  return prml;
}

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
  release(t);
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

void handle_sum_tree(tpalrts_prml& prml) {
  if (prml.front == nullptr) {
    return;
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
}

int main() {
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
