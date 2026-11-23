#include "microtaskparts.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>

using namespace microtaskparts;

auto fib(long n) -> long {
  if (n < 2) {
    return n;
  }
  long a, b;
  fork2join([&] { a = fib(n - 1); }, [&] { b = fib(n - 2); });
  return a + b;
}

int main(int argc, char **argv) {
  long n = (argc > 1) ? std::atol(argv[1]) : 30;
  auto start = std::chrono::steady_clock::now();
  long r = fib(n);
  std::chrono::duration<double> elapsed =
      std::chrono::steady_clock::now() - start;
  std::printf("fib(%ld) = %ld\n", n, r);
  std::printf("workers   %zu\n", get_nb_workers());
  std::printf("exectime  %.3f s\n", elapsed.count());
  return 0;
}
