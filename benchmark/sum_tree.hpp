#pragma once

#include <taskparts/benchmark.hpp>

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
  d.dispatch_or_default("input_tree", "perfect");
}

auto teardown() {
  delete [] n0;
  if (n1 != nullptr) {
    delete [] n1;
  }
}
