{ pkgs ? import <nixpkgs> {} }:

let
  customPython = pkgs.python38.buildEnv.override {
    extraLibs = [ pkgs.python38Packages.jsonschema
                  pkgs.python38Packages.simplejson
                  pkgs.python38Packages.matplotlib
                  pkgs.python38Packages.pathvalidate
                ];
  };
in

pkgs.mkShell {
  buildInputs = [ customPython ];
}
