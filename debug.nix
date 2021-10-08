# Creates an environment prepared for taskparts.
#
# By default, the environment uses clang/llvm instead of GCC. To
# use GCC instead, use the following:
#   $ nix-shell --arg stdenv '(import <nixpkgs> {}).stdenv'
#
# If you don'want to use jemalloc:
#   $ nix-shell --arg jemalloc '(import <nixpkgs> {}).jemalloc450'

{ pkgs   ? import <nixpkgs> {},
  stdenv ? pkgs.clangStdenv,
  hwloc ? pkgs.hwloc,
  jemalloc ? null, # pkgs.jemalloc450
  valgrind ? pkgs.valgrind,
  cilk-stats-rts ? import (pkgs.fetchFromGitHub {
    owner  = "deepsea-inria";
    repo   = "cilk-plus-rts-with-stats";
    rev    = "d143c31554bc9c122d168ec22ed65e7941d4c91d";
    sha256 = "123bsrqcp6kq6xz2rn4bvj2nifflfci7rd9ij82fpi2x6xvvsmsb";
  }) {}
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
    ++ (if valgrind == null then [] else [ valgrind ])
    ++ (if cilk-stats-rts == null then [] else [ cilk-stats-rts ]);

  # jemalloc configuration
  LD_PRELOAD=if jemalloc == null then "" else "${jemalloc}/lib/libjemalloc.so";
  
  # hwloc configuration
  HWLOC_INCLUDE_PREFIX=if hwloc == null then "" else "-DTASKPARTS_HAVE_HWLOC -I${hwloc.dev}/include/";
  HWLOC_LIBRARY_PREFIX=if hwloc == null then "" else "-L${hwloc.lib}/lib/ -lhwloc";
  shellHook = if hwloc == null then "" else ''
    # The line below is needed, at present, because otherwise taskparts may 
    # request more workers than there are cores, which would be incompatible 
    # with the per-core pinning.
    export TASKPARTS_NUM_WORKERS=$( ${hwloc}/bin/hwloc-ls|grep Core|wc -l );
    export CILKRTS_STATS_PREFIX="${cilk-stats-rts-params}"y
  '';

  TASKPARTS_BENCHMARK_WARMUP_SECS=0;
}
