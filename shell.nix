# Creates an environment prepared for taskparts.
#
# In order to build the Cilk Plus benchmarks, the compiler needs
# to be an older version of GCC, such as version 7. It can be
# loaded as follows.
#   $ nix-shell --arg stdenv '(import <nixpkgs> {}).gcc7Stdenv'
# If you want to use Clang, then use the following:
#   $ nix-shell --arg stdenv '(import <nixpkgs> {}).clangStdenv'
# If you want to use OpenCilk, then use the following:
#   $ nix-shell --arg opencilk 'import ./opencilk.nix {}'
#
# If you don't want to use jemalloc:
#   $ nix-shell --arg jemalloc null

# later: investigate idiomatic way to share code between this file and debug.nix

{ pkgs ? import <nixpkgs> {},
  stdenv ? pkgs.stdenv,
  hwloc ? pkgs.hwloc,
  jemalloc ? pkgs.jemalloc,
  cilk-stats-rts ? import ../cilk-plus-rts-with-stats {},
  parlaylib ? import ../parlaylib {},
  pbbsbench ? import ../pbbsbench { parlaylib=parlaylib; },
  chunkedseq ? import ../chunkedseq/script { },
  rollforward ? import ../rollforward {},
  hbtimer-kmod ? import ../heartbeat-linux { pkgs=pkgs; stdenv=stdenv; },
  opencilk ? null
}:

let cilk-stats-rts-params =
      if cilk-stats-rts == null then "" else
        let include-flags =
              "-I ${cilk-stats-rts}/include -DTASKPARTS_CILKRTS_WITH_STATS -Wno-format-zero-length";
            linker-flags =
              "-L ${cilk-stats-rts}/lib -ldl";
        in "${include-flags} ${linker-flags}";
in

stdenv.mkDerivation rec {
  name = "taskparts-benchmark";

  buildInputs =
    [ hwloc ]
    ++ (if jemalloc == null then [] else [ jemalloc ])
    ++ (if cilk-stats-rts == null then [] else [ cilk-stats-rts ]);
  
  # hwloc configuration
  HWLOC_INCLUDE_PREFIX="-DTASKPARTS_HAVE_HWLOC -I${hwloc.dev}/include/";
  HWLOC_LIBRARY_PREFIX="-L${hwloc.lib}/lib/ -lhwloc";

  # heartbeat kernel module extension for the tpal runtime
  HBTIMER_KMOD_INCLUDE_PREFIX=
    if hbtimer-kmod == null then "" else
      "-I ${hbtimer-kmod}/user -DTASKPARTS_TPALRTS_HBTIMER_KMOD";
  HBTIMER_KMOD_LINKER_FLAGS=
    if hbtimer-kmod == null then "" else
      "${hbtimer-kmod}/libhb.so";

  PARLAYLIB_PATH="${parlaylib}";
  PBBSBENCH_PATH="${pbbsbench}";
  CHUNKEDSEQ_PATH="${chunkedseq}";
  TASKPARTS_BENCHMARK_INFILE_PATH="../../../infiles";

  # TPAL/rollforward configuration
  ROLLFORWARD_PATH="${rollforward}";

  # OpenCilk configuration
  OPENCILK_CXX=if opencilk == null then "" else "${opencilk}/bin/clang++";
  
  shellHook = ''
    export NUM_SYSTEM_CORES=$( ${hwloc}/bin/hwloc-ls|grep Core|wc -l )
    export MAKEFLAGS="-j $NUM_SYSTEM_CORES"
    export TASKPARTS_NUM_WORKERS=$NUM_SYSTEM_CORES;
    export CILKRTS_STATS_PREFIX="${cilk-stats-rts-params}"
  '';

  # allow architecture-native optimization mode to be used by the C/C++ compiler
  NIX_ENFORCE_NO_NATIVE=0;
}
