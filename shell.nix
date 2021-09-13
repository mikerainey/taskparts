# Creates an environment where taskparts uses hwloc to do per-core pinning
# and uses jemalloc. For compiler, gcc7 or clang are available.

{ pkgs   ? import <nixpkgs> {},
  stdenv ? pkgs.stdenv,
  clang ? pkgs.clang,
  hwloc ? pkgs.hwloc,
  jemalloc ? pkgs.jemalloc450
}:

pkgs.clangStdenv.mkDerivation rec {
  name = "taskparts-dev";

  buildInputs =
    [ clang hwloc jemalloc ];

  shellHook =
    ''
    export HWLOC_INCLUDE_PREFIX="-DTASKPARTS_HAVE_HWLOC -I${hwloc.dev}/include/"
    export HWLOC_LIBRARY_PREFIX="-L${hwloc.lib}/lib/ -lhwloc"
    # the line below is needed because otherwise taskparts may request more
    # workers than there are cores, which would be incompatible with the
    # per-core pinning
    export TASKPARTS_NUM_WORKERS=$( ${hwloc}/bin/hwloc-ls|grep Core|wc -l )
    export LD_PRELOAD="${jemalloc}/lib/libjemalloc.so"
    '';
}
