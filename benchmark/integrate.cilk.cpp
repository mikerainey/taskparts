#include "cilk.hpp"
#include "integrate.hpp"

int main() {
  size_t n = taskparts::cmdline::parse_or_default_long("n", 1000000000);
  auto f = [&] (double q) -> double {
    return pow(q, 2);
  };
  double start = static_cast<double>(0);
  double end = static_cast<double>(1000);
  taskparts::benchmark_cilk([&] {
    integrate(n, start, end, f);
  });
  return 0;
}
