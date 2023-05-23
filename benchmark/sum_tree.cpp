#include <taskparts/taskparts.hpp>
#include <cstdio>
#include <iostream>
#include <parlay/primitives.h>
#include <parlay/sequence.h>
#include <deque>

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

auto sum(node* n) -> int {
  if (n == nullptr) {
    return 0;
  } else {
    int s0, s1;
    parlay::par_do([&] {
      s0 = sum(n->left);
    }, [&] {
      s1 = sum(n->right);
    });
    return s0 + s1 + n->value;
  }
}

using task = taskparts::vertex;

class tpalrts_prml_node {
public:
  tpalrts_prml_node* prev;
  tpalrts_prml_node* next;
};

class tpalrts_prml {
public:
  tpalrts_prml_node* front = nullptr;
  tpalrts_prml_node* back = nullptr;
  tpalrts_prml() { }
  tpalrts_prml(tpalrts_prml_node* front, tpalrts_prml_node* back)
    : front(front), back(back) { }
};

using kont_tag = enum kont_enum { K1, K2, K3, K4, K5 };

class vhbkont {
public:
  kont_tag tag;
  union {
    struct {
      tpalrts_prml_node prml_node;
      node* n;
    } k1;
    struct {
      int s0;
      node* n;
    } k2;
    struct {
      int* s;
      task* tj;
      std::deque<vhbkont>* kp;
    } k4ork5;
  } u;

  vhbkont(kont_tag tag, node* n, tpalrts_prml_node* prev) // K1
    : tag(tag) { u.k1.n = n; u.k1.prml_node.prev = prev; u.k1.prml_node.next = nullptr; }
  vhbkont(kont_tag tag, int s0, node* n) // K2
    : tag(tag) { u.k2.s0 = s0; u.k2.n = n; }
  vhbkont(kont_tag tag, int* s, task* tj, std::deque<vhbkont>* kp) // K4
    : tag(tag) { u.k4ork5.s = s; u.k4ork5.tj = tj; u.k4ork5.kp = kp; }
  vhbkont(kont_tag tag, int* s, task* tj) // K5
    : tag(tag) { u.k4ork5.s = s; u.k4ork5.tj = tj; u.k4ork5.kp = nullptr; }
  vhbkont(kont_tag tag) // K3
    : tag(tag) { }
};

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
  assert(next != nullptr);
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
  assert(prev != nullptr);
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

auto enclosing_frame_pointer_of(tpalrts_prml_node* n) -> vhbkont* {
  auto r = vhbkont(K1, nullptr, (tpalrts_prml_node*)nullptr);
  auto d = (char*)(&r.u.k1.prml_node)-(char*)(&r);
  auto p = (char*)n;
  auto h = p - d;
  return (vhbkont*)h;
}

auto sum(node *n, std::deque<vhbkont> &k, tpalrts_prml prml) -> void;

template <typename F>
auto create_task(const F& f) -> task* {
  return taskparts::create_vertex(f, taskparts::continuation_minimal);
}

auto fork(task *c, task *k) -> void {
  auto p = taskparts::self();
  auto g = taskparts::capture_continuation(p);
  taskparts::add(k, g);
  taskparts::new_edge(p, k);
  taskparts::new_edge(c, k);
  taskparts::release(c);
  taskparts::release(k);
}

auto join(task* k) -> void {

}
  
auto try_promote(tpalrts_prml prml) -> tpalrts_prml {
  if (prml.front == nullptr) {
    return prml;
  }
  auto f_fr = enclosing_frame_pointer_of(prml.front);
  assert(f_fr->tag == K1);
  auto n = f_fr->u.k1.n;
  auto s = new int[2];
  auto kp = new std::deque<vhbkont>;
  auto tj = create_task([=] {
    std::deque<vhbkont> kj;
    kp->swap(kj);
    *f_fr = vhbkont(K2, s[0] + s[1], n);
    delete kp;
    delete [] s;	    
    sum(nullptr, kj, tpalrts_prml());
  });
  fork(create_task([=] {
    std::deque<vhbkont> k2({vhbkont(K5, s, tj)});
    sum(n->right, k2, tpalrts_prml());
  }), tj);
  prml = tpalrts_prmlist_pop_front(prml);
  *f_fr = vhbkont(K4, s, tj, kp);
  return prml;
}

constexpr
int H = 3000; // heartbeat rate

int answer = -1;

auto sum(node* n, std::deque<vhbkont>& k, tpalrts_prml prml) -> void {
  size_t counter = 0;
  auto heartbeat = [&] () -> bool {
    if (++counter >= H) {
      counter = 0;
      return true;
    }
    return false;
  };
  while (true) {
    if (heartbeat()) { // promotion-ready program point
      prml = try_promote(prml);
    }
    if (n == nullptr) {
      int s = 0;
      while (true) {
	if (heartbeat()) { // promotion-ready program point
	  prml = try_promote(prml);
	}
	auto& f = k.back();
	if (f.tag == K1) {
	  prml = tpalrts_prmlist_pop_back(prml);
	  n = f.u.k1.n->right;
	  f = vhbkont(K2, s, f.u.k1.n);
	  break;
	} else if (f.tag == K2) {
	  s = f.u.k2.s0 + s + f.u.k2.n->value;
	  k.pop_back();
	} else if (f.tag == K3) {
	  answer = s;
	  return;
	} else if (f.tag == K4 || f.tag == K5) {
	  auto i = (f.tag == K4) ? 0 : 1;
	  f.u.k4ork5.s[i] = s;
	  auto tj = f.u.k4ork5.tj;
	  if (f.tag == K4) {
	    f.u.k4ork5.kp->swap(k);
	  }
	  join(tj);
	  return;
	} 
      }
    } else {
      k.push_back(vhbkont(K1, n, prml.back));
      prml = tpalrts_prmlist_push_back(prml, &(k.back().u.k1.prml_node));
      n = n->left;
    }
  }
}

auto sum1(node* n) -> int {
  taskparts::launch_dag_calculus(create_task([=] {
    std::deque<vhbkont> k({vhbkont(K3)});
    sum(n, k, tpalrts_prml());
  }));
  assert(answer != -1);
  return answer;
}

int main(int argc, char* argv[]) {
  auto usage = "Usage: sum_tree <n>";
  if (argc != 2) { printf("%s\n", usage);
  } else {
    long n;
    try { n = std::stol(argv[1]); }
    catch (...) { printf("%s\n", usage); }
    auto child = [&] (size_t nd, size_t i) -> size_t {
      assert((i == 1) || (i == 2));
      return 2 * nd + i;
    };
    auto has_children = [&] (int nd) -> bool {
      return child(nd, 2) < n;
    };
    auto nodes = parlay::to_sequence(parlay::delayed_tabulate(n, [&] (size_t i) {
      return node(1);
    }));
    parlay::parallel_for(0, n, [&] (size_t i) {
      if (! has_children(i)) {
	return;
      }
      auto n1 = &nodes[child(i, 1)];
      auto n2 = &nodes[child(i, 2)];
      assert(child(i, 1) < n);
      assert(child(i, 2) < n);
      new (&nodes[i]) node(1, n1, n2);
    });
    int s = -1;
    taskparts::benchmark([&] {
      s = sum1(&nodes[0]);
    });
    printf("s=%d\n",s);
  }
  return 0;
}
