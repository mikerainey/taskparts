#pragma once

#include "common.hpp"
#include <testData/sequenceData/sequenceData.h>
#include <common/sequenceIO.h>
//#ifndef PARLAY_SEQUENTIAL
#include <benchmarks/classify/decisionTree/classify.C>
//#else
//#include <classify/serial/classify.C>
//#endif

using namespace std;
using namespace benchIO;

features Train;
rows Test;
row labels;
row result;
bool verbose = false;
parlay::sequence<char> comma(1, ',');
parlay::sequence<char> newline(1, '\n');
template <typename Seq>
sequence<char> csv_row(Seq const &e) {
  size_t len = e.size()*2;
  auto ss = parlay::tabulate(len, [&] (size_t i) {
    if (i == len-1) return newline;
    if (i%2 == 1) return comma;
    return parlay::to_chars(e[i/2]);});
  return parlay::flatten(ss);
};

void report_correct(row result, row labels) {
  size_t n = result.size();
  if (n != labels.size()) {
    cout << "size mismatch of results and labels" << endl;
    return;
  }
  size_t num_correct = parlay::reduce(parlay::tabulate(n, [&] (size_t i) {
         return (result[i] == labels[i]) ? 1 : 0;}));
  float percent_correct = (100.0 * num_correct)/n;
  cout << num_correct << " correct out of " << n
       << ", " << percent_correct << " percent" << endl;
}

auto read_row(string filename) {
  auto is_item = [] (char c) -> bool { return c == ',';};
  auto str = parlay::chars_from_file(filename);
  return parlay::map(parlay::tokens(str, is_item), 
		     [] (auto s) -> value {return parlay::chars_to_int(s);});
}

auto read_data(string filename) {
  int num_buckets = 64;
  auto is_line = [] (char c) -> bool { return c == '\r' || c == '\n'|| c == 0;};
  auto is_item = [] (char c) -> bool { return c == ',';};
  auto str = parlay::file_map(filename);
  size_t j = find_if(str,is_line) - str.begin();
  auto head = parlay::make_slice(str).cut(0,j);
  auto rest = parlay::make_slice(str).cut(j+1, str.end()-str.begin());
  auto types = parlay::map(parlay::tokens(head, is_item), [] (auto str) {return str[0];});

  auto process_line = [&] (auto line) {
      auto to_int = [&] (auto x) -> value {
	  int v = parlay::chars_to_int(parlay::to_sequence(x));
	  if (v < 0 || v > max_value) {
	    cout << "entry out range: value = " << v << endl;
	    return 0;
	  }
	  return v;};
      return parlay::map_tokens(line, to_int, is_item);};
  
  return std::pair(types, parlay::map_tokens(rest, process_line, is_line));
}

auto rows_to_features(sequence<char> types, rows const &A) {
  int num_features = types.size();
  size_t num_rows = A.size();

  auto get_feature = [&] (size_t io) {
    int i = (io == 0) ? num_features - 1 : io - 1; // put label at front
    bool is_discrete = (types[i] == 'd');
    auto vals = parlay::tabulate(num_rows, [&] (size_t j) -> value {return A[j][i];});
    int maxv = parlay::reduce(vals, parlay::maxm<char>());
    return feature(is_discrete, maxv+1, vals);
  };

  return parlay::tabulate(num_features, get_feature);
}

auto gen_input() {
  std::string infile_path = "";
  if (const auto env_p = std::getenv("TASKPARTS_BENCHMARK_INFILE_PATH")) {
    infile_path = std::string(env_p);
  }
  force_sequential = taskparts::cmdline::parse_or_default_bool("force_sequential", false);
  parlay::override_granularity = taskparts::cmdline::parse_or_default_long("override_granularity", 0);
  include_infile_load = taskparts::cmdline::parse_or_default_bool("include_infile_load", false);
  auto infile = infile_path + "/" + "kddcup.data";
  auto iFile = taskparts::cmdline::parse_or_default_string("input", infile.c_str());
  string train_file = iFile;
  string test_file = iFile;
  string label_file = iFile;
  auto [types, _Train] = read_data(train_file.append(".train"));
  auto [xtypes, _Test] = read_data(test_file.append(".test"));
  Test = _Test;
  auto labels = read_row(label_file.append(".labels"));
  Train = rows_to_features(types, _Train);
}

auto benchmark_dflt() {
  if (include_infile_load) {
    gen_input();
  }
  result = classify(Train, Test, verbose);
}

auto benchmark() {
  if (force_sequential) {
    benchmark_intermix([&] { benchmark_dflt(); });
  } else {
    benchmark_dflt();
  }
}

auto reset() {
}
