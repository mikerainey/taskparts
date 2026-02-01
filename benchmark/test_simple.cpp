#include <taskparts/taskparts.hpp>
#include <iostream>

// Simple fibonacci function using TaskPaRTS fork2join
long fib(long n) {
  if (n < 2) {
    return n;
  }
  long a, b;
  taskparts::fork2join([&] {
    a = fib(n - 1);
  }, [&] {
    b = fib(n - 2);
  });
  return a + b;
}

int main(int argc, char* argv[]) {
  long n = (argc > 1) ? std::stol(argv[1]) : 10;

  std::cout << "Computing fib(" << n << ") using TaskPaRTS...\n";
  std::cout << "Number of workers: " << taskparts::get_nb_workers() << "\n";

  long result = fib(n);

  std::cout << "Result: " << result << "\n";

  return 0;
}
