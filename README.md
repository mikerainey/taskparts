# TaskPaRTS - A Task-Parallel Run-Time System for C++

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

