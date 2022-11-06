{ pkgs ? import <nixpkgs> {},
  stdenv ? pkgs.stdenv
}:

let
  customPython = pkgs.python310.buildEnv.override {
    extraLibs = [ pkgs.python310Packages.jsonschema
                  pkgs.python310Packages.simplejson
                  pkgs.python310Packages.psutil
                  pkgs.python310Packages.py-cpuinfo
                  pkgs.python310Packages.sqlalchemy
                ];
  };
in

#pkgs.mkShell {
stdenv.mkDerivation rec {
  name = "elastic-benchmark";
  buildInputs = [ customPython ];
}
