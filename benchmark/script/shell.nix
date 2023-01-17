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
                  pkgs.python310Packages.python-sql
                ];
  };
in

#pkgs.mkShell {
stdenv.mkDerivation rec {
  name = "elastic-benchmark";
  buildInputs = [ customPython pkgs.dsq pkgs.jq ];
}
