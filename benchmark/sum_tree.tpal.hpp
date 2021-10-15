#pragma once

#include <deque>

class task;
class node;

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
      task* tk;
    } k3;
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
  vhbkont(kont_tag tag, task* tk) // K3
    : tag(tag) { u.k3.tk = tk; }
};

#ifdef ORIG
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
#endif
