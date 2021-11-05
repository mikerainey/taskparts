{ pkgs ? import <nixpkgs> {} }:

let
  customPython = pkgs.python38.buildEnv.override {
    extraLibs = [ pkgs.python38Packages.jsonschema
                  pkgs.python38Packages.simplejson
                  pkgs.python38Packages.matplotlib
                  pkgs.python38Packages.pathvalidate
                  pkgs.python38Packages.tabulate
                  pkgs.python38Packages.psutil
#                  pkgs.python38Packages.pandas
                ];
  };
in

pkgs.mkShell {
  buildInputs = [ customPython pkgs.pandoc pkgs.texlive.combined.scheme-small ];
}
