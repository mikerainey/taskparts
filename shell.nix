# Creates an environment prepared for taskparts.
#
# By default, the environment uses clang/llvm instead of GCC. To
# use GCC instead, use the following:
#   $ nix-shell --arg stdenv '(import <nixpkgs> {}).stdenv'
#
# If you don't want to use jemalloc:
#   $ nix-shell --arg jemalloc null
# If you don't want to use hwloc:
#   $ nix-shell --arg hwloc null
# Note: if you use hwloc, then the taskparts scheduler will *not* use SMT.

{ pkgs   ? import <nixpkgs> {},
  stdenv ? pkgs.clangStdenv,
  hwloc ? pkgs.hwloc,
  jemalloc ? pkgs.jemalloc450,
  valgrind ? pkgs.valgrind
}:

stdenv.mkDerivation rec {
  name = "taskparts-dev";

  buildInputs =
    [ ]
    ++ (if hwloc == null then [] else [ hwloc ])
    ++ (if jemalloc == null then [] else [ jemalloc ])
    ++ (if valgrind == null then [] else [ valgrind ]);

  shellHook =
    let
      jemallocCfg = 
        if jemalloc == null then ""
        else ''export LD_PRELOAD="${jemalloc}/lib/libjemalloc.so"'';
      hwlocCfg =
        if hwloc == null then ""
        else ''
            export HWLOC_INCLUDE_PREFIX="-DTASKPARTS_HAVE_HWLOC -I${hwloc.dev}/include/"
            export HWLOC_LIBRARY_PREFIX="-L${hwloc.lib}/lib/ -lhwloc"
            # the line below is needed because otherwise taskparts may request more
            # workers than there are cores, which would be incompatible with the
            # per-core pinning
            export TASKPARTS_NUM_WORKERS=$( ${hwloc}/bin/hwloc-ls|grep Core|wc -l )
        '';
    in
    ''
    ${hwlocCfg}
    ${jemallocCfg}
    '';
}
