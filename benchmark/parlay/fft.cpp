#include "common.hpp"
#include <iostream>
#include <string>
#include <random>
#include <cmath>
#include <complex>

#include <parlay/primitives.h>
#include <parlay/random.h>
#include <parlay/internal/get_time.h>

#include "fast_fourier_transform.h"

using complex = std::complex<double>;

parlay::random_generator gen;
std::uniform_real_distribution<double> dis;
parlay::sequence<complex> points;
parlay::sequence<parlay::sequence<complex>> columns;
parlay::sequence<parlay::sequence<complex>> results_c;
long n;

auto gen_input() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  n = std::max((size_t)1, (size_t)taskparts::cmdline::parse_or_default_long("n", 20 * 1000 * 1000));
  parlay::random_generator gen1(0);
  std::uniform_real_distribution<double> dis1(0.0,1.0);
  gen = gen1;
  dis = dis1;
  points = parlay::tabulate(n, [&] (long i) {
    auto r = gen[i];
    return complex{dis(r), dis(r)};});
  int lg = std::floor(std::log2(n));
  n = 1 << lg;
  long lg2 = lg/2;
  long num_columns = 1 << lg2;
  long num_rows = n / num_columns;
    
  columns = parlay::tabulate(num_columns, [&] (long i) {
    return parlay::tabulate(num_rows, [&] (long j) {
      return points[j * num_columns + i];});});
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  results_c = complex_fft_transpose(columns);
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}

int main() {
  parlay::benchmark_taskparts([&] (auto sched) { // benchmark
    benchmark();
  }, [&] (auto sched) { // setup
    gen_input();
  }, [&] (auto sched) { // teardown
#ifndef NDEBUG
    std::cout << "first five points transpose" << std::endl;
    for (long i=0; i < std::min(5l,n); i++)
      std::cout << results_c[i] << std::endl;
#endif
  }, [&] (auto sched) { // reset
    results_c.clear();
  });
  return 0;
}
