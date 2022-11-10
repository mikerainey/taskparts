#pragma once

#include <taskparts/benchmark.hpp>

#include <parlay/sequence.h>
#include <parlay/delayed_sequence.h>
#include <parlay/monoid.h>
#include <parlay/primitives.h>
#include <parlay/parallel.h>

bool include_infile_load;
bool force_sequential;

template <typename B>
auto benchmark_intermix(const B& b) {
  auto nb_repeat = taskparts::cmdline::parse_or_default_long("nb_repeat", 2);
  auto nb_par = taskparts::cmdline::parse_or_default_long("nb_par", 1);
  auto nb_seq = taskparts::cmdline::parse_or_default_long("nb_seq", 1);
  printf("hi\n");
  for (size_t i = 0; i < nb_repeat; i++) {
    taskparts::force_sequential = true;
    for (size_t j = 0; j < nb_seq; j++) {
      b();
    }
    taskparts::force_sequential = false;
    for (size_t j = 0; j < nb_par; j++) {
      b();
    }
  }
}
