#pragma once

#include "common.hpp"
#include "common/get_time.h"
#include "parlay/io.h"

auto grep_delayed(parlay::sequence<char> const& str,
		  parlay::sequence<char> const& search_str) {
  timer t("grep");
  size_t n = str.size();
  auto line_break = [] (char c) {return c == '\n';};
  auto idx = parlay::block_delayed::filter(parlay::iota(n), [&] (size_t i) -> long {
      return line_break(str[i]);});
  t.next("filter");
  size_t m = idx.size();
  //cout << m << endl;
  auto y = parlay::delayed_tabulate(m+1, [&] (size_t i) {
      size_t start = (i==0 ? 0 : idx[i-1]);
      size_t end = (i==m ? n : idx[i]);
      return parlay::make_slice(str.begin()+start,str.begin()+end);
    });
  auto r = parlay::filter(y, [&] (auto x) {
      return parlay::search(x, search_str) != x.end();});
  t.next("filter 2");
  return r;
}

size_t result;
parlay::sequence<char> input;
parlay::sequence<char> pattern;

auto gen_input() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  auto infile = taskparts::cmdline::parse_or_default_string("input", "sources");
  auto pattern_str = taskparts::cmdline::parse_or_default_string("pattern", "xxy");
  pattern = parlay::tabulate(pattern_str.size(), [&] (size_t i) { return pattern_str[i]; });
  input = parlay::chars_from_file(infile.c_str(), true);
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  auto out_str = grep_delayed(input, pattern);
  result = out_str.size();
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}
