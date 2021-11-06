#pragma once

#include <parlay/delayed_sequence.h>
#include <parlay/monoid.h>
#include <parlay/primitives.h>
#include <parlay/parallel.h>
#include <testData/sequenceData/sequenceData.h>
#include <common/sequenceIO.h>
#ifndef PARLAY_SEQUENTIAL
#include <removeDuplicates/parlayhash/dedup.h>
#else
#include <removeDuplicates/serial_sort/dedup.h>
#endif

using str = parlay::sequence<char>;

parlay::sequence<int> a_ints;
parlay::sequence<int> res_ints;
parlay::sequence<str> a_strs;
parlay::sequence<str> res_strs;
benchIO::elementType in_type;
size_t dflt_n = 10000000;
size_t n = dflt_n;
bool include_infile_load;

auto parse_ints(auto In) {
  a_ints = benchIO::parseElements<int>(In.cut(1, In.size()));
  n = a_ints.size();
}
auto parse_strs(auto In) {
  a_strs = benchIO::parseElements<str>(In.cut(1, In.size()));
  n = a_strs.size();
}

auto gen_input() {
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  if (include_infile_load) {
    auto input = taskparts::cmdline::parse_or_default_string("input", "");
    if (input == "") {
      exit(-1);
    }
    auto infile = input + ".seq";
    auto In = benchIO::get_tokens(infile.c_str());
    in_type = benchIO::elementTypeFromHeader(In[0]);
    if (in_type == benchIO::intType) {
      parse_ints(In);
    } else if (in_type == benchIO::stringT) {
      parse_strs(In);
    } else {
      taskparts_die("bogus input");
    }
    return;
  }
  n = taskparts::cmdline::parse_or_default_long("n", dflt_n);
  taskparts::cmdline::dispatcher d;
  d.add("random_ints", [&] {
    in_type = benchIO::intType;
    size_t r = taskparts::cmdline::parse_or_default_long("r", n);
    a_ints = dataGen::randIntRange<int>((size_t) 0,n,r);
  });
  d.add("strings", [&] {
    in_type = benchIO::stringT;
    taskparts_die("todo");
  });
  d.dispatch_or_default("input", "random_ints");
}

auto _benchmark() {
  if (in_type == benchIO::intType) {
    res_ints = dedup(a_ints);
  } else if (in_type == benchIO::stringT) {
    res_strs = dedup(a_strs);
  }
}

auto benchmark_no_nudges() {
  if (include_infile_load) {
    gen_input();
  }
  _benchmark();
}

auto benchmark_with_nudges(size_t repeat) {
  size_t nudges = taskparts::cmdline::parse_or_default_long("nudges", n/2);
  for (size_t i = 0; i < repeat; i++) {
    for (size_t j = 0; j < nudges; j++) {
      auto k = taskparts::hash(i + j) % n;
      if (in_type == benchIO::intType) {
	a_ints[k]++;
      } else if (in_type == benchIO::stringT) {
	auto& s = a_strs[k];
	auto m = s.size();
	if (m > 0) {
	  auto k2 = taskparts::hash(k) % m;
	  s[k2]++;
	}
      }
    }
    _benchmark();    
  }
}

auto benchmark() {
  size_t repeat = taskparts::cmdline::parse_or_default_long("repeat", 0);
  if (repeat == 0) {
    benchmark_no_nudges();
  } else {
    benchmark_with_nudges(repeat);
  }
}
