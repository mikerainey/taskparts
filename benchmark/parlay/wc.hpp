#pragma once

#include "common.hpp"

std::string s;

// ------------------------- Word Count -----------------------------

template <class Seq>
std::tuple<size_t,size_t,size_t> wc(Seq const &s) {
  // Create a delayed sequence of pairs of integers:
  // the first is 1 if it is line break, 0 otherwise;
  // the second is 1 if the start of a word, 0 otherwise.
  auto f = parlay::delayed_seq<std::pair<size_t, size_t>>(s.size(), [&] (size_t i) {
    bool is_line_break = s[i] == '\n';
    bool word_start = ((i == 0 || std::isspace(s[i-1])) && !std::isspace(s[i]));
    return std::make_pair<size_t,size_t>(is_line_break, word_start);
  });

  // Reduce summing the pairs to get total line breaks and words.
  // This is faster than summing them separately since that would
  // require going over the input sequence twice.
  auto m = parlay::pair_monoid(parlay::addm<size_t>{}, parlay::addm<size_t>{});
  auto r = parlay::reduce(f, m);

  return std::make_tuple(r.first, r.second, s.size());
}

auto gen_input() {
  size_t n = taskparts::cmdline::parse_or_default_long("n", 500 * 1000 * 1000);
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  s.resize(n, 'b');
}


auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  wc(s);
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}
