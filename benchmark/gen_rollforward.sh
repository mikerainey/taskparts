#!/usr/bin/env bash

# arg1: benchmark name, arg2 : label prefix

$CXX gen_rollforward.cpp -o gen_rollforward

file=$1
file_orig="$1_orig.s"
file_manual="$1_manual.s"
file_rf_map="$1_rollforward_map.hpp"
file_rf_decls="$1_rollforward_decls.hpp"

cmd="./gen_rollforward -file $file_orig -prefix $2"

$cmd > $file_manual
$cmd --only_table > $file_rf_map
$cmd --only_header > $file_rf_decls
