#include "fib_nativeforkjoin.hpp"
#include <cstdio>
#include <iostream>

int main(int argc, char* argv[]) {
  auto usage = "Usage: fib <n>";
  if (argc != 2) { printf("%s\n", usage);
  } else {
    long n;
    try { n = std::stol(argv[1]); }
    catch (...) { printf("%s\n", usage); }
    long r;
    if (const auto env_p = std::getenv("FIB_THRESHOLD")) {
      fib_serial_threshold = std::max(2l, std::stol(env_p));
    }
    taskparts::benchmark([&] {
      r = fib_nativeforkjoin(n);
    });
    printf("fib(%ld) = %ld\n", n, r);
  }
  return 0;
}
