# Creates an environment prepared for taskparts.
#
# By default, the environment uses clang/llvm instead of GCC. To
# use GCC instead, use the following:
#   $ nix-shell --arg stdenv '(import <nixpkgs> {}).stdenv'
# If you need to use Cilk, then use the following:
#   $ nix-shell --arg stdenv '(import <nixpkgs> {}).gcc7Stdenv'
#
# If you don't want to use jemalloc:
#   $ nix-shell --arg jemalloc null
# For full debugging:
#   $ nix-shell --arg stdenv '(import <nixpkgs> {}).stdenv' --arg jemalloc null

# later: investigate idiomatic way to share code between this file and debug.nix

{ pkgs   ? import <nixpkgs> {},
  stdenv ? pkgs.clangStdenv,
  hwloc ? pkgs.hwloc,
  jemalloc ? pkgs.jemalloc450,
  cilk-stats-rts ? import ../cilk-plus-rts-with-stats {}
}:

let cilk-stats-rts-params =
      if cilk-stats-rts == null then "" else
        ''-L ${cilk-stats-rts}/lib -I ${cilk-stats-rts}/include -ldl -DTASKPARTS_CILKRTS_WITH_STATS -Wno-format-zero-length'';
in

stdenv.mkDerivation rec {
  name = "taskparts-benchmark";

  buildInputs =
    [ hwloc ]
    ++ (if jemalloc == null then [] else [ jemalloc ])
    ++ (if cilk-stats-rts == null then [] else [ cilk-stats-rts ]);
  
  # jemalloc configuration
  LD_PRELOAD=if jemalloc == null then "" else "${jemalloc}/lib/libjemalloc.so";
  
  # hwloc configuration
  HWLOC_INCLUDE_PREFIX="-DTASKPARTS_HAVE_HWLOC -I${hwloc.dev}/include/";
  HWLOC_LIBRARY_PREFIX="-L${hwloc.lib}/lib/ -lhwloc";
  shellHook = ''
    # The line below is needed, at present, because otherwise taskparts may 
    # request more workers than there are cores, which would be incompatible 
    # with the per-core pinning.
    export TASKPARTS_NUM_WORKERS=$( ${hwloc}/bin/hwloc-ls|grep Core|wc -l );
    export CILKRTS_STATS_PREFIX="${cilk-stats-rts-params}"
  '';

  # allow architecture-native optimization mode to be used by the C/C++ compiler
  NIX_ENFORCE_NO_NATIVE=0;
}
