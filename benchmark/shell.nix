# Creates a shell environment prepared for debugging taskparts.
#
# By default, the environment uses GCC instead of clang/llvm. To
# use clang/llvm (version 13) instead, use the following:
#   $ nix-shell --arg stdenv '(import <nixpkgs> {}).llvmPackages_13.stdenv'
# or for the default version
#   $ nix-shell --arg stdenv '(import <nixpkgs> {}).clangStdenv'

# To use Cilk:
# nix-shell --arg opencilk 'import ./../../nix-packages/pkgs/opencilk/default.nix {}'

{ pkgs   ? import <nixpkgs> {},
  stdenv ? pkgs.llvmPackages_14.stdenv,
  hwloc ? pkgs.hwloc, # can be null
  gdb ? pkgs.gdb, # can be null
  valgrind ? pkgs.valgrind, # can be null
  parlaylib ? import ./../../parlaylib/default.nix {}, # can be null
  cmdline ? import ./../../nix-packages/pkgs/cmdline { stdenv=pkgs.stdenv; fetchgit=pkgs.fetchgit; pandoc=null; texlive=null; }, # can be null
  jemalloc ? pkgs.jemalloc,
  opencilk ? null
}:

stdenv.mkDerivation rec {
  name = "taskparts-debugging-shell";

  buildInputs = builtins.filter (x: x != null) [hwloc gdb valgrind];

  HWLOC_CFLAGS=if hwloc == null then "" else "-DTASKPARTS_USE_HWLOC -I${hwloc.dev}/include/";
  HWLOC_LIBFLAGS=if hwloc == null then "" else "-L${hwloc.lib}/lib/ -lhwloc";

  PARLAYLIB_CFLAGS=if parlaylib == null then "" else "-I${parlaylib}/include -I${parlaylib}/share/examples";
  
  CMDLINE_CFLAGS=if cmdline == null then "" else "-I${cmdline}/include/";

  TASKPARTS_DEBUG_CFLAGS=pkgs.lib.concatMapStringsSep " " (x: "-DTASKPARTS_" + x + "=1") ["RUN_UNIT_TESTS" "USE_VALGRIND"];
  TASKPARTS_OPTIONAL_FLAGS=pkgs.lib.concatStringsSep " " [HWLOC_CFLAGS PARLAYLIB_CFLAGS CMDLINE_CFLAGS];
  TASKPARTS_OPTIONAL_LINKER_FLAGS=HWLOC_LIBFLAGS;
  TASKPARTS_BENCHMARK_WARMUP_SECS=0;

  JEMALLOC_PATH=if jemalloc == null then "" else "${jemalloc}";
  
  LD_LIBRARY_PATH="./bin";

  OPENCILK_CXX=if opencilk == null then "" else "${opencilk}/bin/clang++";
  
  shellHook = if hwloc == null then "" else ''
    export NUM_SYSTEM_CORES=$( ${hwloc}/bin/hwloc-ls|grep Core|wc -l )
    export MAKEFLAGS="-j $NUM_SYSTEM_CORES"
    export TASKPARTS_NUM_WORKERS=$NUM_SYSTEM_CORES;
    export PARLAY_NUM_THREADS=$NUM_SYSTEM_CORES;
    export LD_PRELOAD=$JEMALLOC_PATH/lib/libjemalloc.so
  '';
}
