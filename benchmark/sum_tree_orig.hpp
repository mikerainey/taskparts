// compiled w/ clang 12.0.1
// -DNDEBUG -O3 -march=znver2 -fno-verbose-asm -m64 -fno-stack-protector -fno-asynchronous-unwind-tables -fomit-frame-pointer

#include <vector>
#include <tuple>

extern
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
 
auto sum(node* n, std::vector<vkont>& k) -> void {
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
  std::vector<vkont> k({vkont(K3)});
  sum(n, k);
}

using task = void;

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
void join(task*);

extern
bool heartbeat;

static constexpr
int nullprml = -1;

auto prmlist_push_back(std::vector<vhbkont>& k, int prmlfr, int prmlbk, int t) -> std::pair<int, int> {
  if (prmlbk != nullprml) {
    k[prmlbk].u.k1.next = t;
  }
  prmlbk = t;
  if (prmlfr == nullprml) {
    prmlfr = prmlbk;
  }
  return std::make_pair(prmlfr, prmlbk);
}

auto prmlist_pop_back(std::vector<vhbkont>& k, int prmlfr, int prmlbk) -> std::pair<int, int> {
  int next = prmlbk;
  int prev = k[next].u.k1.prev;
  if (prev == nullprml) {
    prmlfr = nullprml;
  } else {
    k[prev].u.k1.next = nullprml;
    k[next].u.k1.prev = nullprml;
  }
  prmlbk = prev;
  return std::make_pair(prmlfr, prmlbk);
}

extern
std::pair<int, int> sum_tree_heartbeat_handler(std::vector<vhbkont>& k, int prmlfr, int prmlbk);
  
void sum_heartbeat(node* n, std::vector<vhbkont>& k, int prmlfr, int prmlbk) {
  while (true) {
    if (heartbeat) { // promotion-ready program point
      std::tie(prmlfr, prmlbk) = sum_tree_heartbeat_handler(k, prmlfr, prmlbk);
    }
    if (n == nullptr) {
      int s = 0;
      while (true) {
	if (heartbeat) { // promotion-ready program point
	  std::tie(prmlfr, prmlbk) = sum_tree_heartbeat_handler(k, prmlfr, prmlbk);
	}
	auto& f = k.back();
	if (f.tag == K1) {
	  std::tie(prmlfr, prmlbk) = prmlist_pop_back(k, prmlfr, prmlbk);
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
      k.push_back(vhbkont(K1, n, prmlbk));
      std::tie(prmlfr, prmlbk) = prmlist_push_back(k, prmlfr, prmlbk, k.size() - 1);
      n = n->left;
    }
  }
}
  
/*
  auto sum(node* n, task* tk) -> void {
    scheduler::launch(new task([&] {
      std::vector<vhbkont> k({vhbkont(K3, tk)});
      sum(n, k, nullprml, nullprml);
    }));
  } */
