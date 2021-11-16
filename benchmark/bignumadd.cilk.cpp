#include "cilk.hpp"
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
  size_t n = std::max((size_t)1, (size_t)taskparts::cmdline::parse_or_default_long("n", 100 * 1000 * 1000));
  a = parlay::tabulate(n, [] (size_t i) -> byte {return 64;});
  b = parlay::tabulate(n, [] (size_t i) -> byte {return 64;});
}

int main() {
  taskparts::benchmark_cilk([&] { // benchmark
    benchmark();
  }, [&] { // setup
    gen_input();
  }, [&] { // teardown
    std::cout << "result1 " << result1 << std::endl;
    std::cout << "result2 " << result2 << std::endl;
  }, [&] { // reset

  });
  return 0;
}
