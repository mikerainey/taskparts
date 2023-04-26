# Creates a shell environment prepared for debugging taskparts.
#
# By default, the environment uses clang/llvm instead of GCC. To
# use GCC instead, use the following:
#   $ nix-shell --arg stdenv '(import <nixpkgs> {}).stdenv'

{ pkgs   ? import <nixpkgs> {},
  stdenv ? pkgs.stdenv,
  cmake ? pkgs.cmake
}:

stdenv.mkDerivation rec {
  name = "taskparts";

  src = ./.;

  buildInputs = [ cmake ];
  
}
