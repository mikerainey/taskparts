{ pkgs ? import <nixpkgs> {} }:

let
  customPython = pkgs.python38.buildEnv.override {
    extraLibs = [ pkgs.python38Packages.jsonschema pkgs.python38Packages.simplejson ];
  };
in

pkgs.mkShell {
  buildInputs = [ customPython ];
}
