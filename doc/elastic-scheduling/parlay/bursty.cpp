// A ParlayLib program whose parallelism comes in bursts. A round runs fib(n)
// once with no parallelism and once with plenty, so half of the work of a
// run is serial. Written like the examples of ParlayLib, so that it builds
// with any scheduler ParlayLib supports, and times each run the way they do.
//
//   bursty <n> <rounds>

#include <iostream>
#include <string>

#include <parlay/internal/get_time.h>
#include <parlay/parallel.h>

__attribute__((noinline)) long fib_serial(long n) {
  return (n < 2) ? n : fib_serial(n - 1) + fib_serial(n - 2);
}

long fib(long n) {
  if (n < 20) return fib_serial(n);
  long a, b;
  parlay::par_do([&] { a = fib(n - 1); }, [&] { b = fib(n - 2); });
  return a + b;
}

int main(int argc, char* argv[]) {
  auto usage = "Usage: bursty <n> <rounds>";
  if (argc != 3) std::cout << usage << std::endl;
  else {
    long n, rounds;
    try { n = std::stol(argv[1]); rounds = std::stol(argv[2]); }
    catch (...) { std::cout << usage << std::endl; return 1; }

    // Read at every use, so that the compiler computes each phase where it
    // is written.
    volatile long input = n;

    parlay::internal::timer t("Time");
    long result = 0;
    for (int i = 0; i < 3; i++) {
      t.start();
      for (long r = 0; r < rounds; r++) {
        result += fib_serial(input);  // a phase with no parallelism
        result += fib(input);         // a phase with plenty
      }
      t.next("bursty");
    }

    std::cout << "sum of results: " << result << std::endl;
  }
}
