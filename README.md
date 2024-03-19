# TaskPaRTS - A Task-Parallel Run-Time System for C++

## Building

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

