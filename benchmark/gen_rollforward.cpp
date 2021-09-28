#include <string>
#include <fstream>
#include <streambuf>
#include <sstream>
#include <set>
#include <vector>
#include <deque>

#include "../include/taskparts/cmdline.hpp"

int main() {
  auto mk_rollforward_label = [] (const std::string& s) {
    return s + "_rf";
  };

  std::string file = taskparts::cmdline::parse_or_default_string("file", "");
  std::string prefix = taskparts::cmdline::parse_or_default_string("prefix", file);
  bool only_table = taskparts::cmdline::parse_or_default_bool("only_table", false);
  bool only_header = taskparts::cmdline::parse_or_default_bool("only_header", false);

  if (file == "") {
    std::cout << "bogus input file " << file << std::endl;
    return 1;
  }
  
  std::ifstream t(file);
  std::string asm_str((std::istreambuf_iterator<char>(t)),
		      std::istreambuf_iterator<char>());
  
  auto is_label = [] (std::string& s) {
    auto n = s.size();
    return (n >= 2) && (s[n - 1] == ':');
  };

  auto label_of_decl = [] (std::string& s) {
    return std::string(s, 0, s.size() - 1);
  };

  std::set<std::string> source_labels;
  
  auto mk_label_set = [&] (std::string& s) {
    std::istringstream iss(s);
    for (std::string line; std::getline(iss, line); ) {
      if (is_label(line)) {
	auto l = label_of_decl(line);
	source_labels.insert(l);
      }
    }
  };

  mk_label_set(asm_str);

  auto is_asm_lexeme = [] (char c) {
    return
      (c == '(') ||
      (c == ')') ||
      (c == '$') ||
      (c == ',') ||
      (c == '\t') ||
      (c == ' ');
  };

  auto lex_asm_line = [&] (const std::string& line) {
    std::deque<std::string> tokens;
    std::string cur;
    for (std::size_t i = 0; i < line.size(); i++) {
      if (is_asm_lexeme(line[i])) {
	tokens.push_back(cur);
	cur = "";
	tokens.push_back(line.substr(i, 1));
      } else {
	cur += line.substr(i, 1);
      }
    }
    if (cur != "") {
      tokens.push_back(cur);
    }
    return tokens;
  };
  
  auto get_nb_instrs = [&] (const std::string& asm_str) {
    std::size_t instr_line_nb = 0;
    std::istringstream iss(asm_str);
    for (std::string line; std::getline(iss, line); ) {
      if (! is_label(line)) {
	instr_line_nb++;
      }
    }
    return instr_line_nb;
  };

  std::size_t total_nb_instrs = get_nb_instrs(asm_str);

  auto mk_instr_label = [&] (std::size_t i) {
    return prefix + std::to_string(i);
  };

  auto mk_instr_labels = [&] (std::size_t nb_instrs) -> std::vector<std::string> {
    std::vector<std::string> result;
    for (std::size_t i = 0; i < total_nb_instrs; i++) {
      result.push_back(mk_instr_label(i));
    }
    return result;
  };

  auto remove_comment_from_line = [] (std::string& line) {
    auto pos = line.find_first_of("#");
    if (pos == std::string::npos) {
      return line;
    }
    return line.erase(pos);
  };

  auto relabel_line = [&] (const std::string& line) {
    auto tokens = lex_asm_line(line);
    std::string result;
    for (auto t : tokens) {
      if (source_labels.find(t) == source_labels.end()) {
	result += t;
      } else {
	result += mk_rollforward_label(t);
      }
    }
    return result;
  };

  auto gen_asm_body = [&] (std::string& asm_str, auto relabel_instr_label, auto relabel_line) {
    std::string result;
    std::size_t instr_line_nb = 0; // not counting lines w/ labels
    std::istringstream iss(asm_str);
    for (std::string line; std::getline(iss, line); ) {
      line = remove_comment_from_line(line);
      if (is_label(line)) {
	auto l = label_of_decl(line);
	result += relabel_line(l) + ":\n";
      } else {
	auto instr_label = relabel_instr_label(mk_instr_label(instr_line_nb++));
	auto line2 = instr_label + ":" + relabel_line(line) + "\n";
	result += line2;
      }
    }
    return result;
  };

  auto gen_label_decl = [&] (const std::string& l) {
    std::cout << ".globl  " << l << std::endl;
    std::cout << ".type  " << l << ", @function" << std::endl;
  };

  auto gen_asm_decls = [&] {
    for (auto l : source_labels) { 
      if (l[0] == '.') {
	continue;
      }
      gen_label_decl(l);
      gen_label_decl(mk_rollforward_label(l));
    }
    for (auto l : mk_instr_labels(total_nb_instrs)) {
      gen_label_decl(l);
      gen_label_decl(mk_rollforward_label(l));
    }
  };

  auto gen_asm = [&] {
    std::cout << ".text" << std::endl;
    std::cout << ".p2align 4,,15" << std::endl;
    gen_asm_decls();
    std::cout << gen_asm_body(asm_str,
			      [&] (const std::string& s) { return s; },
			      [&] (const std::string& s) { return s; })
	      << std::endl;
    std::cout << gen_asm_body(asm_str,
			      [&] (const std::string& s) { return mk_rollforward_label(s); },
			      [&] (const std::string& s) { return relabel_line(s); })
	      << std::endl;
  };

  auto gen_table_entry = [&] (const std::string& l) {
    std::cout << "mk_rollforward_entry(" << l << "," << mk_rollforward_label(l) << ")," << std::endl;
  };

  auto gen_table = [&] {
    for (auto l : mk_instr_labels(total_nb_instrs)) {
      gen_table_entry(l);
    }
  };

  auto gen_header_entry = [&] (const std::string& l) {
    std::cout << "extern \"C\" void " << l << "();" << std::endl;
  };

  auto gen_header = [&] {
    for (auto l : mk_instr_labels(total_nb_instrs)) {
      gen_header_entry(l);
      gen_header_entry(mk_rollforward_label(l));
    }
  };

  if (only_table) {
    gen_table();
    return 0; 
  }

  if (only_header) {
    gen_header();
    return 0; 
  }

  gen_asm();
  
  return 0;
}

