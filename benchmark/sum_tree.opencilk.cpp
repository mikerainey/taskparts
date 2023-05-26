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

int threshold = 11;

auto sum_cilk(node* n, int d) -> int {
  if (n == nullptr) {
    return 0;
  } else if (d >= threshold) {
    return sum_cilk(n->left, d+1) + sum_cilk(n->right, d+1) + n->value;
  } else {
    int s0, s1;
    parlay::par_do([&] {
      s0 = sum_cilk(n->left, d+1);
    }, [&] {
      s1 = sum_cilk(n->right, d+1);
    });
    return s0 + s1 + n->value;
  }
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

static inline
auto now() -> std::chrono::time_point<std::chrono::steady_clock> {
  return std::chrono::steady_clock::now();
}

static inline
auto diff(std::chrono::time_point<std::chrono::steady_clock> start,
          std::chrono::time_point<std::chrono::steady_clock> finish) -> double {
  std::chrono::duration<double> elapsed = finish - start;
  return elapsed.count();
}

static inline
auto since(std::chrono::time_point<std::chrono::steady_clock> start) -> double {
  return diff(start, now());
}

template <
typename Benchmark,
typename Setup = std::function<void()>,
typename Teardown = std::function<void()>,
typename Reset = std::function<void()>>
auto benchmark_opencilk(const Benchmark& benchmark,
			const Setup& setup = [] {},
			const Teardown& teardown = [] {},
			const Reset& reset = [] {}) -> void {
  setup();
  int nb = 5;
  for (size_t i = 0; i < nb; i++) {
    benchmark();
  }
  std::string outfile = "stdout";
  if (const auto env_p = std::getenv("TASKPARTS_BENCHMARK_STATS_OUTFILE")) {
    outfile = std::string(env_p);
  }
  FILE* f = (outfile == "stdout") ? stdout : fopen(outfile.c_str(), "w");
  fprintf(f, "[\n");
  nb = 1; // for now, let's just do one run after warmup
  for (size_t i = 0; i < nb; i++) {
    auto b = now();
    benchmark();
    fprintf(f, "{\"exectime\": %f}",since(b));
    fprintf(f, (i+1 == nb) ? "\n" : ",\n");
  }
  fprintf(f, "]\n");
  if (f != stdout) {
    fclose(f);
  }
  //  instrumentation_report(outfile);
  teardown();
}

int main() {
  using namespace deepsea;
  size_t height = cmdline::parse_or_default_int("height", 23);
  threshold = cmdline::parse_or_default_int("threshold", threshold);
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
  d.add("opencilk", [&] {
    benchmark_opencilk([&] {
      s = sum_rec(root);
    });
  });
  d.add("opencilk_gc", [&] {
    benchmark_opencilk([&] {
      s = sum_cilk(root, 0);
    });
  });
  d.dispatch_or_default("algorithm", "opencilk");
  printf("sum\t%d\n", s);
  return 0;
}
