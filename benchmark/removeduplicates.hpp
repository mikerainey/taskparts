#pragma once

#include "common.hpp"
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

auto parse_ints(auto In) {
  a_ints = benchIO::parseElements<int>(In.cut(1, In.size()));
  n = a_ints.size();
}
auto parse_strs(auto In) {
  a_strs = benchIO::parseElements<str>(In.cut(1, In.size()));
  n = a_strs.size();
}

auto gen_input() {
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  //if (include_infile_load) {
    auto input = taskparts::cmdline::parse_or_default_string("input", "random_int");
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
    //}
    /*  n = taskparts::cmdline::parse_or_default_long("n", dflt_n);
  taskparts::cmdline::dispatcher d;
  d.add("random_int", [&] {
    in_type = benchIO::intType;
    size_t r = taskparts::cmdline::parse_or_default_long("r", n);
    a_ints = dataGen::randIntRange<int>((size_t) 0,n,r);
  });
  d.add("strings", [&] {
    in_type = benchIO::stringT;
    taskparts_die("todo");
  });
  d.dispatch_or_default("input", "random_int"); */
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  if (in_type == benchIO::intType) {
    res_ints = dedup(a_ints);
  } else if (in_type == benchIO::stringT) {
    res_strs = dedup(a_strs);
  }
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}

auto reset() {
  if (in_type == benchIO::intType) {
    res_ints.clear(); 
  } else if (in_type == benchIO::stringT) {
    res_strs.clear();
  }
}
