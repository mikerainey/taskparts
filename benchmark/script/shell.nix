{ pkgs ? import <nixpkgs> {} }:

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

pkgs.mkShell {
  buildInputs = [ customPython ];
}
