# Creates a shell environment prepared for debugging taskparts.
#
# By default, the environment uses clang/llvm instead of GCC. To
# use GCC instead, use the following:
#   $ nix-shell --arg stdenv '(import <nixpkgs> {}).stdenv'

{ pkgs   ? import <nixpkgs> {},
  stdenv ? pkgs.stdenv,
  hwloc ? pkgs.hwloc # can be null
}:

stdenv.mkDerivation rec {
  name = "taskparts";

  src = ./.;

  buildInputs = builtins.filter (x: x != null) [hwloc];

  HWLOC_CFLAGS=if hwloc == null then "" else "-DTASKPARTS_USE_HWLOC -I${hwloc.dev}/include/";
  HWLOC_LIBFLAGS=if hwloc == null then "" else "-L${hwloc.lib}/lib/ -lhwloc";

  TASKPARTS_OPTIONAL_FLAGS=HWLOC_CFLAGS;
  TASKPARTS_LINKER_FLAGS=HWLOC_LIBFLAGS;

  buildPhase = ''
    make -j -C src libraries
  '';

  installPhase = ''
    make -C src TASKPARTS_INSTALL_PATH=$out install
  '';
  
}
