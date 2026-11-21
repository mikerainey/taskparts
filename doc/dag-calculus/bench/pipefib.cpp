// pipe-fib: the n-th Fibonacci number, in binary, by a pipeline.
//
// Iteration i computes F(i) = F(i-1) + F(i-2), from the least significant
// bit up, `grain` bits per stage. Stage j of iteration i reads the bits that
// stage j of iteration i-1 wrote, so it waits for that stage: the pipeline
// is a grid of nodes in which each depends on the one before it in its
// iteration and on the one beside it in the previous iteration. The work is
// quadratic in n, and the longest chain of dependencies is linear.
//
// This is the synthetic benchmark of Lee et al. (SPAA 2013): with one bit
// per stage it is their pipe-fib, which measures what a node of a pipeline
// costs, and with 256 bits per stage their pipe-fib-256.
//
// usage: pipefib <serial|dag> [n] [grain] [throttle] [check]

#include "harness.hpp"
#include "pipe.hpp"

#include <vector>

using namespace bench;

using bits = std::vector<uint8_t>; // one bit per element, least first

// A bound on the number of bits of F(i): F(i) < 2^(0.69425 i).
static size_t nb_bits(size_t i) { return i * 69425 / 100000 + 2; }

// Bits lo to hi of x + y into z, where x has nx bits and y has ny <= nx.
// Returns the carry out.
__attribute__((noinline)) static unsigned
add_bits(const uint8_t *x, size_t nx, const uint8_t *y, size_t ny, uint8_t *z,
         size_t lo, size_t hi, unsigned carry) {
  size_t k = lo;
  for (; (k < hi) && (k < ny); k++) {
    unsigned s = x[k] + y[k] + carry;
    z[k] = (uint8_t)(s & 1);
    carry = s >> 1;
  }
  for (; (k < hi) && (k < nx); k++) {
    unsigned s = x[k] + carry;
    z[k] = (uint8_t)(s & 1);
    carry = s >> 1;
  }
  for (; k < hi; k++) {
    z[k] = (uint8_t)carry;
    carry = 0;
  }
  return carry;
}

// F(i) is kept in number i modulo the count of numbers.
struct numbers {
  std::vector<bits> ring;
  explicit numbers(size_t count, size_t n) : ring(count, bits(nb_bits(n), 0)) {
    ring[1 % count][0] = 1; // F(1); F(0) is all zeros
  }
  uint8_t *of(size_t i) { return ring[i % ring.size()].data(); }
};

static void fib_serial(numbers &f, size_t n, size_t grain) {
  for (size_t i = 2; i <= n; i++) {
    unsigned carry = 0;
    for (size_t lo = 0; lo < nb_bits(i); lo += grain) {
      carry = add_bits(f.of(i - 1), nb_bits(i - 1), f.of(i - 2),
                       nb_bits(i - 2), f.of(i), lo,
                       std::min(lo + grain, nb_bits(i)), carry);
    }
  }
}

// F(i) is overwritten by iteration i + throttle + 2 of the pipeline, which
// starts only once iteration i + 2, the last to read F(i), has finished.
static void fib_dag(numbers &f, size_t n, size_t grain, size_t throttle) {
  pipeline::pipe_while<size_t>(
      throttle,
      [&](size_t k, size_t &i) {
        i = k + 2;
        return i <= n;
      },
      [&](pipeline::stages &s, size_t &i) {
        unsigned carry = 0;
        long j = 1;
        for (size_t lo = 0; lo < nb_bits(i); lo += grain, j++) {
          s.wait(j);
          carry = add_bits(f.of(i - 1), nb_bits(i - 1), f.of(i - 2),
                           nb_bits(i - 2), f.of(i), lo,
                           std::min(lo + grain, nb_bits(i)), carry);
        }
      },
      [](size_t &) {});
}

// F(n) modulo a prime, from its bits.
static uint64_t residue(const uint8_t *x, size_t nb) {
  const uint64_t p = 1000000007;
  uint64_t r = 0;
  for (size_t k = nb; k > 0; k--) {
    r = (2 * r + x[k - 1]) % p;
  }
  return r;
}

int main(int argc, char **argv) {
  const char *version = (argc > 1) ? argv[1] : "dag";
  size_t n = (size_t)arg(argc, argv, 2, 60000);
  size_t grain = (size_t)arg(argc, argv, 3, 256);
  size_t throttle = (size_t)arg(argc, argv, 4, 0);
  bool check = wants_check(argc, argv, 5);
  bool parallel = !is(version, "serial");
  if ((n < 2) || (grain < 1) || (parallel && !is(version, "dag"))) {
    std::fprintf(stderr, "usage: pipefib <serial|dag> [n >= 2] [grain >= 1] "
                         "[throttle] [check]\n");
    return 2;
  }
  if (throttle == 0) {
    throttle = 4 * get_nb_workers();
  }
  numbers f(parallel ? throttle + 2 : 3, n);
  region r = measure(parallel, [&] {
    if (parallel) {
      fib_dag(f, n, grain, throttle);
    } else {
      fib_serial(f, n, grain);
    }
  });
  uint64_t result = residue(f.of(n), nb_bits(n));
  std::printf("pipefib %s n=%zu grain=%zu throttle=%zu\n", version, n, grain,
              throttle);
  std::printf("result    F(n) mod 1000000007 = %llu\n",
              (unsigned long long)result);
  report(r);
  if (check) {
    // Independently of the bits: the same recurrence, modulo the prime.
    uint64_t a = 0, b = 1;
    for (size_t i = 2; i <= n; i++) {
      uint64_t c = (a + b) % 1000000007;
      a = b;
      b = c;
    }
    return report_check(result == b);
  }
  return 0;
}
