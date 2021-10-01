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
  valgrind ? pkgs.valgrind
}:

stdenv.mkDerivation rec {
  name = "taskparts-debug";

  buildInputs =
    [ ]
    ++ (if hwloc == null then [] else [ hwloc ])
    ++ (if jemalloc == null then [] else [ jemalloc ])
    ++ (if valgrind == null then [] else [ valgrind ]);

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
  '';

  TASKPARTS_BENCHMARK_WARMUP_SECS=0  
}
