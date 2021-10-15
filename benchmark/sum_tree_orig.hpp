// compiled w/ clang 12.0.1
// -DNDEBUG -O3 -march=znver2 -fno-verbose-asm -m64 -fno-stack-protector -fno-asynchronous-unwind-tables -fomit-frame-pointer

#define ORIG
#include "sum_tree.tpal.hpp"

extern
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

extern
void join(task*);

extern
bool heartbeat;

extern
tpalrts_prml sum_tree_heartbeat_handler(tpalrts_prml);

void sum_heartbeat(node* n, std::deque<vhbkont>& k, tpalrts_prml prml) {
  while (true) {
    if (heartbeat) { // promotion-ready program point
      prml = sum_tree_heartbeat_handler(prml);
    }
    if (n == nullptr) {
      int s = 0;
      while (true) {
	if (heartbeat) { // promotion-ready program point
	  prml = sum_tree_heartbeat_handler(prml);
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
  
/*
  auto sum(node* n, task* tk) -> void {
    scheduler::launch(new task([&] {
      std::deque<vhbkont> k({vhbkont(K3, tk)});
      sum(n, k, nullprml, nullprml);
    }));
  } */
