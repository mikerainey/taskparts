#include <algorithm>
#include <deque>
#include <cstdio>
#include "sum_tree.hpp"

extern
void handle_sum_tree(tpalrts_prml& prml);

void rollforward_handler_annotation __rf_handle_sum_tree(tpalrts_prml& prml) {
  handle_sum_tree(prml);
  rollbackward
}

void sum_heartbeat(node* n, std::deque<vhbkont>& k, tpalrts_prml prml) {
  while (true) {
    if (n == nullptr) {
      int s = 0;
      while (true) {
	__rf_handle_sum_tree(prml);
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
	  join(f.u.k3.tk);
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
