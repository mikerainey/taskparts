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
                  (with import <nixpkgs> {}; pkgs.python310Packages.callPackage ../../../flexibench {})
                ];
  };
in

stdenv.mkDerivation rec {
  name = "elastic-benchmark";
  buildInputs = [ customPython pkgs.dsq pkgs.jq ];
}
