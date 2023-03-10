#pragma once

#include "rollforward.h"

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
int answer;

class task;
extern auto join(task *) -> void;

class vhbkont;

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

extern
void rollforward_handler_annotation __rf_handle_sum_tree(tpalrts_prml&);

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

extern
auto tpalrts_prmlist_push_back(tpalrts_prml prml, tpalrts_prml_node* t) -> tpalrts_prml;    
extern
auto tpalrts_prmlist_pop_back(tpalrts_prml prml) -> tpalrts_prml;
