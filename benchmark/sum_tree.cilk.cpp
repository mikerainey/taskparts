#include <vector>
#include <stdio.h>

#include "cilk.hpp"
#include "sum_tree.hpp"

auto sum(node* n) -> int {
  if (n == nullptr) {
    return 0;
  } else {
    auto s0 = cilk_spawn sum(n->left);
    auto s1 =             sum(n->right);
    cilk_sync;
    return s0 + s1 + n->value;
  }
}

int main() {
  return -1; // for now, this benchmark crashes on most large inputs
  taskparts::benchmark_cilk([&] { // benchmark
    answer = sum(n0);
  }, [&] { gen_input(taskparts::bench_scheduler()); }, [&] { teardown(); });
  
  printf("answer=%d\n", answer);
  return 0;
}
