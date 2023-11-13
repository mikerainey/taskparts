#include "benchmark.hpp"
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <iostream>
#include <numeric> // for std::gcd

class rational {
public:
    rational(int64_t numerator=1, int64_t denominator=1) : num(numerator), den(denominator) {
        normalize();
    }

    // Normalizes the rational number by ensuring the denominator is positive and
    // the fraction is reduced to its simplest form.
    void normalize() {
        if (den < 0) {
            num = -num;
            den = -den;
        }
        int64_t gcd = std::gcd(num, den);
        num /= gcd;
        den /= gcd;
    }

    // Adds two rational numbers and returns the result as a new rational object.
    rational operator+(const rational& rhs) const {
        int64_t common_denominator = std::lcm(den, rhs.den);
        int64_t numerator_sum = (num * (common_denominator / den)) + (rhs.num * (common_denominator / rhs.den));
        return rational(numerator_sum, common_denominator);
    }

  bool operator<(const rational& rhs) const {
    // Cross multiply to compare without converting to floats
    return num * rhs.den < rhs.num * den;
  }

  int64_t ceiling() const {
        // For positive numbers, we can use std::ceil after converting to a floating point.
        // For negative numbers, the ceiling is just the division of num by den.
        if (num > 0) {
            return static_cast<int64_t>(std::ceil(static_cast<double>(num) / den));
        } else {
            return num / den - (num % den == 0 ? 0 : 1);
        }
    }

  static rational fromString(const std::string& str) {
        std::size_t slashPos = str.find('/');
        if (slashPos == std::string::npos) {
            throw std::invalid_argument("Invalid rational number format");
        }
        int64_t numerator = std::stoi(str.substr(0, slashPos));
        int64_t denominator = std::stoi(str.substr(slashPos + 1));
        return rational(numerator, denominator);
    }

    // Outputs the rational number to a stream in the form "numerator/denominator".
    friend std::ostream& operator<<(std::ostream& out, const rational& r) {
        out << r.num << "/" << r.den;
        return out;
    }

    int64_t num; // Numerator
    int64_t den; // Denominator
};

//namespace taskparts {
  
template <typename F, typename R, typename V>
auto reduce(const F& f, const R& r, V z, size_t lo, size_t hi, size_t grain = 2) -> V {
  if (lo == hi) {
    return z;
  }
  if ((hi - lo) <= grain) {
    for (auto i = lo; i < hi; i++) {
      z = r(f(lo), z);
    }
    return z;
  }
  V rs[2];
  auto mid = (lo + hi) / 2;
  parlay::par_do([&] { rs[0] = reduce(f, r, z, lo, mid); },
		 [&] { rs[1] = reduce(f, r, z, mid, hi); }); 
  return r(rs[0], rs[1]); 
}

inline
uint64_t _hash(uint64_t u) {
  uint64_t v = u * 3935559000370003845ul + 2691343689449507681ul;
  v ^= v >> 21;
  v ^= v << 37;
  v ^= v >>  4;
  v *= 4768777513237032717ul;
  v ^= v << 20;
  v ^= v >> 41;
  v ^= v <<  5;
  return v;
}

auto oscillate(rational max_n, rational step, size_t grain) -> void {
  auto secs = 4.0;
  auto s = now();
  uint64_t nb_apps = 0;
  uint64_t r = 1234;
  rational n = rational(1, 1);
  uint64_t rounds = 0;
  std::cout << "max=" << max_n << " step=" << step << std::endl;
  while (since(s) < secs) {
    r = reduce([&] (uint64_t i) { return _hash(i); },
	       [&] (uint64_t r1, uint64_t r2) { return _hash(r1 | r2); },
	       r, 0, n.ceiling(), grain);
    //    std::cout << "n=" << n << " step=" << step << std::endl;
    nb_apps += n.ceiling();
    n = n + step;
    if (!( n < max_n)) {
      step.num *= -1l;
      n = rational(1, 1);
    } else if ((n + 1) < rational(1,1)) {
      step.num *= -1l;
      n = max_n;
    }
    rounds++;
  }
  std::cout << "nb_apps=" << nb_apps << " rounds=" << rounds << " r=" << r << std::endl;
}

//} // namespace taskparts

int main(int argc, char* argv[]) {
  auto usage = "Usage: oscillator <max_n> <step> <grain>";
  if (argc != 4) { printf("%s\n", usage);
  } else {
    rational max_n;
    rational step;
    uint64_t grain = 2;
    try { max_n = rational::fromString(argv[1]); }
    catch (...) { printf("%s\n", usage); }
    try { step = rational::fromString(argv[2]); }
    catch (...) { printf("%s\n", usage); }
    try { grain = std::stol(argv[3]); }
    catch (...) { printf("%s\n", usage); }
    parlay::par_do([&] { // to ensure that the taskparts scheduler is running
      taskparts::benchmark([&] {
	oscillate(max_n, step, grain);
      });
    }, [&] {});

  }
  return 0;
}
