#pragma once

#include "common.hpp"

double start = static_cast<double>(0);
double end = static_cast<double>(1000);

// ---------------------------- Integration ----------------------------

template <typename F>
double integrate(size_t num_samples, double start, double end, F f) {
   double delta = (end-start)/num_samples;
   auto samples = parlay::delayed_seq<double>(num_samples, [&] (size_t i) {
        return f(start + delta/2 + i * delta); });
   return delta * parlay::reduce(samples, parlay::addm<double>());
}

auto gen_input() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
}


auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  size_t n = taskparts::cmdline::parse_or_default_long("n", 2000 * 1000 * 1000);
  auto f = [&] (double q) -> double {
    return pow(q, 2);
  };
  integrate(n, start, end, f);
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}
