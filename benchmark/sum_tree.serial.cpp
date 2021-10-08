#include <deque>
#include <stdio.h>

#include <taskparts/benchmark.hpp>
#include "sum_tree.hpp"

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

auto sum(node* n, std::deque<vkont>& k) -> void {
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

void sum_serial(node* n) {
  std::deque<vkont> k({vkont(K3)});
  sum(n, k);
}

auto sum(node* n) -> int {
  if (n == nullptr) {
    return 0;
  } else {
    auto s0 = /* spawn */ sum(n->left);
    auto s1 =             sum(n->right);
    /* sync; */
    return s0 + s1 + n->value;
  }
}

int main() {
  
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    taskparts::cmdline::dispatcher d;
    d.add("iterative", [&] {
      sum_serial(n0);
    });
    d.add("recursive", [&] {
      answer = sum(n0);
    });
    d.dispatch_or_default("algorithm", "iterative");
  }, [&] (auto sched) { gen_input(sched); }, [&] (auto sched) { teardown(); });
  
  printf("answer=%d\n", answer);
  return 0;
}
