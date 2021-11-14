#pragma once

#include <taskparts/benchmark.hpp>
#include <parlay/delayed_sequence.h>
#include <parlay/monoid.h>
#include <parlay/primitives.h>
#include <parlay/parallel.h>

bool include_infile_load;
bool force_sequential;

template <typename B>
auto benchmark_intermix(const B& b) {
  auto m = taskparts::cmdline::parse_or_default_long("m", 2);
  auto k = taskparts::cmdline::parse_or_default_long("k", 2);
  for (size_t i = 0; i < k; i++) {
    taskparts::force_sequential = true;
    for (size_t j = 0; j < m; j++) {
      b();
    }
    taskparts::force_sequential = false;
    b();
  }
}
