# TaskPaRTS - A Task-Parallel Run-Time System for C++

## Taskparts compiler flags:

Platform (required):
- TASKPARTS_POSIX boolean
- TASKPARTS_DARWIN boolean

Architecture (required):
- TASKPARTS_ARM64 boolean
- TASKPARTS_X64 boolean

CPU pinning:
- TASKPARTS_USE_HWLOC boolean

Elastic scheduling
- TASKPARTS_DISABLE_ELASTIC boolean

Instrumentation:
- TASKPARTS_STATS boolean
- TASKPARTS_LOGGING boolean

Diagnostics:
- TASKPARTS_META_SCHEDULER_SERIAL_RANDOM boolean
- TASKPARTS_USE_VALGRIND boolean
- TASKPARTS_RUN_UNIT_TESTS boolean

Work stealing:
- TASKPARTS_USE_CHASELEV_DEQUE boolean

