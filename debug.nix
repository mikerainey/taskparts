# Creates an environment prepared for taskparts.
#
# By default, the environment uses clang/llvm instead of GCC. To
# use GCC instead, use the following:
#   $ nix-shell --arg stdenv '(import <nixpkgs> {}).stdenv'
#
# If you don'want to use jemalloc:
#   $ nix-shell --arg jemalloc '(import <nixpkgs> {}).jemalloc450'

{ pkgs   ? import <nixpkgs> {},
  stdenv ? pkgs.stdenv,
  hwloc ? pkgs.hwloc,
  jemalloc ? null, # pkgs.jemalloc450
  gdb ? pkgs.gdb,
  valgrind ? pkgs.valgrind,
  cilk-stats-rts ? import (pkgs.fetchFromGitHub {
    owner  = "deepsea-inria";
    repo   = "cilk-plus-rts-with-stats";
    rev    = "d143c31554bc9c122d168ec22ed65e7941d4c91d";
    sha256 = "123bsrqcp6kq6xz2rn4bvj2nifflfci7rd9ij82fpi2x6xvvsmsb";
  }) {},
  parlaylib ? import ../parlaylib {}
}:

let cilk-stats-rts-params =
      if cilk-stats-rts == null then "" else
        ''-L ${cilk-stats-rts}/lib -I ${cilk-stats-rts}/include -ldl -DTASKPARTS_CILKRTS_WITH_STATS -Wno-format-zero-length'';
in

stdenv.mkDerivation rec {
  name = "taskparts-debug";

  buildInputs =
    [ ]
    ++ (if hwloc == null then [] else [ hwloc ])
    ++ (if jemalloc == null then [] else [ jemalloc ])
    ++ (if gdb == null then [] else [ gdb ])
    ++ (if valgrind == null then [] else [ valgrind ])
    ++ (if cilk-stats-rts == null then [] else [ cilk-stats-rts ]);

  # jemalloc configuration
  LD_PRELOAD=if jemalloc == null then "" else "${jemalloc}/lib/libjemalloc.so";
  
  # hwloc configuration
  HWLOC_INCLUDE_PREFIX=if hwloc == null then "" else "-DTASKPARTS_HAVE_HWLOC -I${hwloc.dev}/include/";
  HWLOC_LIBRARY_PREFIX=if hwloc == null then "" else "-L${hwloc.lib}/lib/ -lhwloc";
  shellHook = if hwloc == null then "" else ''
    export NUM_SYSTEM_CORES=$( ${hwloc}/bin/hwloc-ls|grep Core|wc -l )
    export MAKEFLAGS="-j $NUM_SYSTEM_CORES"
    export TASKPARTS_NUM_WORKERS=$NUM_SYSTEM_CORES;
    export CILKRTS_STATS_PREFIX="${cilk-stats-rts-params}"
  '';

  PARLAYLIB_PATH="${parlaylib}";

  TASKPARTS_BENCHMARK_WARMUP_SECS=0;
}
