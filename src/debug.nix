# Creates a shell environment prepared for debugging taskparts.
#
# By default, the environment uses clang/llvm instead of GCC. To
# use GCC instead, use the following:
#   $ nix-shell --arg stdenv '(import <nixpkgs> {}).stdenv'

{ pkgs   ? import <nixpkgs> {},
  stdenv ? pkgs.stdenv,
  hwloc ? pkgs.hwloc, # can be null
  gdb ? pkgs.gdb, # can be null
  valgrind ? pkgs.valgrind # can be null
}:

stdenv.mkDerivation rec {
  name = "taskparts-debugging-shell";

  buildInputs = builtins.filter (x: x != null) [hwloc gdb valgrind];

  HWLOC_CFLAGS=if hwloc == null then "" else "-DTASKPARTS_USE_HWLOC -I${hwloc.dev}/include/";
  HWLOC_LIBFLAGS=if hwloc == null then "" else "-L${hwloc.lib}/lib/ -lhwloc";

  TASKPARTS_OPTIONAL_FLAGS="-DTASKPARTS_RUN_UNIT_TESTS=1 " + HWLOC_CFLAGS;
  TASKPARTS_LINKER_FLAGS=HWLOC_LIBFLAGS;
  
  shellHook = if hwloc == null then "" else ''
    export NUM_SYSTEM_CORES=$( ${hwloc}/bin/hwloc-ls|grep Core|wc -l )
    export MAKEFLAGS="-j $NUM_SYSTEM_CORES"
    export TASKPARTS_NUM_WORKERS=$NUM_SYSTEM_CORES;
  '';

  TASKPARTS_BENCHMARK_WARMUP_SECS=0;
}