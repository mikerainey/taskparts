#include <taskparts/taskparts.hpp>
#include <cstdio>
#include <iostream>
#include <parlay/primitives.h>
#include <parlay/sequence.h>
#include <deque>
#include <cmdline.hpp>

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

auto sum_rec(node* n) -> int {
  if (n == nullptr) {
    return 0;
  } else {
    int s0, s1;
    parlay::par_do([&] {
      s0 = sum_rec(n->left);
    }, [&] {
      s1 = sum_rec(n->right);
    });
    return s0 + s1 + n->value;
  }
}

int answer = -1;

using kont_tag = enum kont_enum { K1, K2, K3, K4, K5 };

class vkont {
public:
  kont_tag tag;
  union {
    struct {
      node* n;
    } k1;
    struct {
      int s0;
      node* n;
    } k2;
  } u;

  vkont(kont_tag tag, node* n) // K1
    : tag(tag) { u.k1.n = n; }
  vkont(kont_tag tag, int s0, node* n) // K2
    : tag(tag) { u.k2.s0 = s0; u.k2.n = n; }
  vkont(kont_tag tag) // K3
    : tag(tag) { }
};

template <typename Seq=std::deque<vkont>>
auto sum_iterative(node* n, Seq& k) -> void {
  while (true) {
    if (n == nullptr) {
      int s = 0;
      while (true) {
	auto& f = k.back();
	if (f.tag == K1) {
	  n = f.u.k1.n->right;
	  f = vkont(K2, s, f.u.k1.n);
	  break;
	} else if (f.tag == K2) {
	  s = f.u.k2.s0 + s + f.u.k2.n->value;
	  k.pop_back();
	} else if (f.tag == K3) {
	  answer = s;
	  return;
	}
      }
    } else {
      k.push_back(vkont(K1, n));
      n = n->left;
    }
  }
}

template <typename Seq=std::deque<vkont>>
auto sum_iterative(node* n) -> int {
  Seq k({vkont(K3)});
  sum_iterative<Seq>(n, k);
  return answer;
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

auto sum_heartbeat_loop(node *n, std::deque<vhbkont> &k, tpalrts_prml prml) -> void;

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
    sum_heartbeat_loop(nullptr, kj, tpalrts_prml());
  });
  fork(create_task([=] {
    std::deque<vhbkont> k2({vhbkont(K5, s, tj)});
    sum_heartbeat_loop(n->right, k2, tpalrts_prml());
  }), tj);
  prml = tpalrts_prmlist_pop_front(prml);
  *f_fr = vhbkont(K4, s, tj, kp);
  return prml;
}

int H = 8000; // heartbeat rate

auto sum_heartbeat_loop(node* n, std::deque<vhbkont>& k, tpalrts_prml prml) -> void {
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

auto sum_heartbeat(node* n) -> int {
  taskparts::launch_dag_calculus(create_task([=] {
    std::deque<vhbkont> k({vhbkont(K3)});
    sum_heartbeat_loop(n, k, tpalrts_prml());
  }));
  assert(answer != -1);
  return answer;
}

auto gen_perfect_tree(int height) -> parlay::sequence<node> {
  auto n = 1 << height;
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
  return nodes;
}

std::function<node*(std::deque<node>&)> default_update_leaf = [] (std::deque<node>& path) -> node* {
  path.push_back(node(1));
  return &path.back();
 };

auto gen_random_path_update(node* root, size_t& rng, std::deque<node>& path,
			    std::function<node*(std::deque<node>&)> update_leaf = default_update_leaf) -> void {
  auto hash = [] (uint64_t u) -> uint64_t {
    uint64_t v = u * 3935559000370003845ul + 2691343689449507681ul;
    v ^= v >> 21;
    v ^= v << 37;
    v ^= v >>  4;
    v *= 4768777513237032717ul;
    v ^= v << 20;
    v ^= v >> 41;
    v ^= v <<  5;
    return v;
  };
  std::function<node*(node*)> ins_rec;
  ins_rec = [&] (node* n) -> node* {
    if (n == nullptr) {
      return update_leaf(path);
    }
    rng = hash(rng);
    auto branch = rng % 2;
    auto child = (branch == 0) ? n->left : n->right;
    path.push_back(node(1));
    auto& n2 = path.back();
    auto n3 = ins_rec(child);
    ((branch == 0) ? n2.left : n2.right) = n3;
    ((branch == 1) ? n2.left : n2.right) = child;
    return &n2;
  };
  ins_rec(root);
}

auto gen_random_updates(node* root, size_t n, std::deque<std::deque<node>>& updates,
			std::function<node*(std::deque<node>&)> update_leaf = default_update_leaf) -> void {
  node* _root = root;
  size_t rng = 1243;
  for (size_t i = 0; i < n; i++) {
    updates.push_back(std::deque<node>());
    auto& u = updates.back();
    gen_random_path_update(_root, rng, u, update_leaf);
    _root = &u[0];
  }
}

auto gen_linear_path(size_t length, std::deque<node>& path, size_t branch = 0) -> node* {
  if (length == 0) {
    return nullptr;
  }
  path.push_back(node(1));
  auto n0 = &path.back();
  auto n = n0;
  for (size_t i = 1; i < length; i++) {
    path.push_back(node(1));
    auto n2 = &path.back();
    ((branch == 0) ? n->left : n->right) = n2;
    n = n2;
  }
  return n0;
}

namespace taskparts {
template <
typename Benchmark,
typename Setup = std::function<void()>,
typename Teardown = std::function<void()>,
typename Reset = std::function<void()>>
auto benchmark_opencilk(const Benchmark& benchmark,
			const Setup& setup = [] {},
			const Teardown& teardown = [] {},
			const Reset& reset = [] {}) -> void {
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
    //reset_scheduler([&] { // worker local

      //}, [&] { // global
      instrumentation_reset();
      instrumentation_start();
      instrumentation_on_enter_work();
      //}, false);
    benchmark();
    //reset_scheduler([&] { // worker local
      instrumentation_on_exit_work();
      //}, [&] { // global
      instrumentation_capture();
      if ((i + 1) < get_benchmark_nb_repeat()) {
        reset();
      }
      instrumentation_start();
      //}, true);
  }
  std::string outfile = "stdout";
  if (const auto env_p = std::getenv("TASKPARTS_BENCHMARK_STATS_OUTFILE")) {
    outfile = std::string(env_p);
  }
  instrumentation_report(outfile);
  teardown();
}
}

int main() {
  using namespace deepsea;
  size_t height = cmdline::parse_or_default_int("height", 23);
  H = cmdline::parse_or_default_int("heartbeat_rate", H);
  auto nb_updates = std::max(1, cmdline::parse_or_default_int("nb_updates", 1));
  auto update_path_length = std::max(1, cmdline::parse_or_default_int("update_path_length", 1));
  auto branch = cmdline::parse_or_default_int("branch", 0) % 2;
  std::deque<std::deque<node>> updates;
  auto nodes = gen_perfect_tree(height);
  gen_random_updates(&nodes[0], nb_updates, updates, [&] (std::deque<node>& path) -> node* {
    return gen_linear_path(update_path_length, path, branch);
  });
  auto& node0 = updates.back();
  auto root = &node0[0];
  int s = -1;
  deepsea::cmdline::dispatcher d;
  d.add("recursive", [&] {
    taskparts::benchmark([&] {
      s = sum_rec(root);
    });
  });
  d.add("iterative", [&] {
    taskparts::benchmark([&] {
      s = sum_iterative(root);
    });
  });
  d.add("heartbeat", [&] {
    taskparts::benchmark([&] {
      s = sum_heartbeat(root);
    });
  });
  d.add("opencilk", [&] {
    taskparts::benchmark_opencilk([&] {
      s = sum_rec(root);
    });
  });
  d.dispatch_or_default("algorithm", "iterative");
  printf("sum\t%d\n", s);
  return 0;
}
