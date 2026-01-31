# TaskPaRTS - A Task-Parallel Run-Time System for C++

## Getting Started with Nix Flakes (Recommended)

TaskPaRTS provides a Nix flake for reproducible development environments with all dependencies included (LLVM 18, hwloc, parlaylib, jemalloc, gdb, valgrind).

### Enter the development shell:
~~~~
$ nix develop --extra-experimental-features 'nix-command flakes'
~~~~

This automatically configures:
- C++ compiler (LLVM 18 with clang++)
- Build tools (CMake, Make)
- All environment variables (HWLOC_CFLAGS, PARLAYLIB_CFLAGS, etc.)
- Parallel build settings based on detected CPU cores

### Build benchmarks:
~~~~
$ nix develop --extra-experimental-features 'nix-command flakes'
$ cd benchmark
$ make fib.header_opt
$ ./bin/fib.header_opt 35
~~~~

### Build the library with CMake:
~~~~
$ nix develop --extra-experimental-features 'nix-command flakes'
$ cmake -S . -B build -DHWLOC=ON
$ cmake --build build
~~~~

### Install TaskPaRTS:
~~~~
$ nix build --extra-experimental-features 'nix-command flakes'
$ nix profile install . --extra-experimental-features 'nix-command flakes'
~~~~

## Building (Without Nix)

CMake options:
- `-DNONELASTIC=ON` Disable elastic scheduling.
- `-DSTATS=ON` Enable collecting and reporting of stats on scheduling behavior.
- `-DLOGGING=ON` Enable collecting and reporting of logging on scheduling behavior.
- `-DHWLOC=ON` Build with hwloc, which enables CPU pinning.

## Header-file library (recommended for best performance)

~~~~
$ cmake -S . -B build -DTASKPARTS_HEADER_ONLY=ON
~~~~

### Linked library

~~~~
$ cmake -S . -B build
~~~~

## Taskparts compiler flags:

Platform (required):
- TASKPARTS_POSIX
- TASKPARTS_DARWIN

Architecture (required):
- TASKPARTS_ARM64
- TASKPARTS_X64

CPU pinning:
- TASKPARTS_USE_HWLOC

Elastic scheduling
- TASKPARTS_DISABLE_ELASTIC

Instrumentation:
- TASKPARTS_STATS
- TASKPARTS_LOGGING

Diagnostics:
- TASKPARTS_META_SCHEDULER_SERIAL_RANDOM
- TASKPARTS_USE_VALGRIND
- TASKPARTS_RUN_UNIT_TESTS

Work stealing:
- TASKPARTS_USE_CHASELEV_DEQUE

