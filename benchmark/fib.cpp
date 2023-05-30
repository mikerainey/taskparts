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
    taskparts::benchmark([&] {
      r = fib_nativeforkjoin(n);
    });
    printf("fib(%ld) = %ld\n", n, r);
  }
  return 0;
}
