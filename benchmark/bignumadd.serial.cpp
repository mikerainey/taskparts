#include <taskparts/benchmark.hpp>
#include "bignumadd.hpp"

int result1, result2;
parlay::sequence<byte> a;
parlay::sequence<byte> b;

auto benchmark() {
  auto [sums, carry] = big_add_delayed(a,b);
  result1 = sums[0];
  result2 = carry;
}

auto gen_input() {
  size_t n = std::max((size_t)1, (size_t)taskparts::cmdline::parse_or_default_long("n", 100000000));
  a = parlay::tabulate(n, [] (size_t i) -> byte {return 64;});
  b = parlay::tabulate(n, [] (size_t i) -> byte {return 64;});
}

int main() {
  parlay::benchmark_taskparts([&] (auto sched) { // benchmark
    benchmark();
  }, [&] (auto sched) { // setup
    gen_input();
  }, [&] (auto sched) { // teardown
    std::cout << "result1 " << result1 << std::endl;
    std::cout << "result2 " << result2 << std::endl;
  }, [&] (auto sched) { // reset

  });
  return 0;
}
