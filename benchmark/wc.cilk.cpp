#include "cilk.hpp"
#include "wc.hpp"

int main() {
  size_t n = taskparts::cmdline::parse_or_default_long("n", 100000000);
  std::string s;
  taskparts::benchmark_cilk([&] {
    wc(s);
  }, [&] {
    s.resize(n, 'b');
  });
  return 0;
}
