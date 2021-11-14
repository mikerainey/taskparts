#include "pdfs.hpp"

int main() {
  parlay::benchmark_taskparts([&] (auto sched) { // benchmark
    benchmark();
  }, [&] (auto sched) { // setup
    load_input();
  }, [&] (auto sched) { // teardown
    size_t nb_visited = parlay::reduce(parlay::delayed_map(visited, [&] (auto& p) -> size_t {
      return (size_t)p.load();}));
      cout << "total visited = " << nb_visited << endl;
  }, [&] (auto sched) { // reset
    visited.clear();
  });
  return 0;
}
