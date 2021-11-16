#include "cilk.hpp"
#include "bfs.hpp"

int main() {
  taskparts::benchmark_cilk([&] { // benchmark
    benchmark();
  }, [&] { // setup
    load_input();
  }, [&] { // teardown
    size_t visited = parlay::reduce(parlay::delayed_map(parents, [&] (auto p) -> size_t {
      return (p == -1) ? 0 : 1;}));
    cout << "total visited = " << visited << endl;
  }, [&] { // reset
    parents.clear();
  });
  return 0;
}
