#include <iostream>
#include <string>
#include <random>

#include <parlay/primitives.h>
#include <parlay/sequence.h>
#include <parlay/random.h>
#include <parlay/internal/get_time.h>

#include "nbody_fmm.h"
#include "common.hpp"

long n;
parlay::sequence<particle> particles;

// checks 500 randomly selected points for accuracy
void check_accuracy(sequence<particle>& p) {
  long n = p.size();
  long nCheck = std::min<long>(n, 500);
  parlay::random_generator gen(123);
  std::uniform_int_distribution<long> dis(0, n-1);

  auto errors = parlay::tabulate(nCheck, [&] (long i) {
    auto r = gen[i];
    long idx = dis(r);
    vect3d force;
    for (long j=0; j < n; j++)
      if (idx != j) {
        vect3d v = (p[j].pt) - (p[idx].pt);
        double r = v.length();
        force = force + (v * (p[j].mass * p[idx].mass / (r*r*r)));
      }
    return (force - p[idx].force).length()/force.length();});

  double rms_error = std::sqrt(parlay::reduce(parlay::map(errors, [] (auto x) {return x*x;}))/nCheck);
  double max_error = parlay::reduce(errors, parlay::maximum<double>());
  std::cout << "  Sampled RMS Error: " << rms_error << std::endl;
  std::cout << "  Sampled Max Error: " << max_error << std::endl;
}

auto gen_input() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  n = std::max((size_t)1, (size_t)taskparts::cmdline::parse_or_default_long("n", 1 * 1000 * 1000));
  // generate n random particles in a square around the origin with
  // random masses from 0 to 1.
  parlay::random_generator gen(0);
  std::uniform_real_distribution<> box_dis(-1.0,1.0);
  std::uniform_real_distribution<> mass_dis(0.0,1.0);
  particles = parlay::tabulate(n, [&] (long i) {
    auto r = gen[i];
    point3d pt{box_dis(r), box_dis(r), box_dis(r)};
    return particle{pt, mass_dis(r), vect3d()};});
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  forces(particles, ALPHA);
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
    check_accuracy(particles);
#endif
  }, [&] (auto sched) { // reset
    particles.clear();
    gen_input();
  });
  return 0;
}
