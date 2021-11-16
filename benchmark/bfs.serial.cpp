#include <taskparts/benchmark.hpp>
#include "bfs.hpp"

int main() {
  parlay::benchmark_taskparts([&] (auto sched) { // benchmark
    benchmark();
  }, [&] (auto sched) { // setup
    load_input();
  }, [&] (auto sched) { // teardown
    size_t visited = parlay::reduce(parlay::delayed_map(parents, [&] (auto p) -> size_t {
      return (p == -1) ? 0 : 1;}));
    cout << "total visited = " << visited << endl;
  }, [&] (auto sched) { // reset
    parents.clear();
  });
  return 0;
}
