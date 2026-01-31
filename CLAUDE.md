# TaskPaRTS Project Guide for AI Assistants

This document provides a comprehensive overview of the TaskPaRTS project for AI assistants working with this codebase.

## Project Overview

**TaskPaRTS** (Task-Parallel Run-Time System) is a C++ library for task-parallel programming with work-stealing scheduling.

- **Author**: Mike Rainey
- **License**: MIT
- **Language**: C++17
- **Primary Use**: Parallel algorithm implementations with fork-join parallelism
- **Repository**: https://github.com/mikerainey/taskparts

### Key Characteristics
- Header-only library (recommended) or linked library
- Work-stealing scheduler with elastic scheduling support
- Native continuation families for low-level control
- Platform-specific optimizations (POSIX/Darwin, X64/ARM64)
- Optional hwloc integration for CPU pinning and topology awareness
- Integration with ParlayLib for parallel algorithms

## Project Structure

```
taskparts/
├── flake.nix                      # Nix flake for reproducible dev environment
├── flake.lock                     # Dependency lockfile
├── CMakeLists.txt                 # CMake build configuration (3.14+)
├── README.md                      # User documentation
├── DEVELOPMENT.md                 # Developer notes and TODOs
├── CLAUDE.md                      # This file - AI assistant guide
├── LICENSE                        # MIT license
├── .gitignore                     # Git ignore patterns
├── nix/
│   └── parlaylib.nix             # ParlayLib package definition
├── include/taskparts/
│   ├── taskparts.hpp             # Public API (269 bytes)
│   └── taskparts_internal.hpp    # Implementation details (15KB)
├── src/
│   └── taskparts.cpp             # Single compilation unit (79KB)
├── cmake/
│   └── FindHWLOC.cmake           # CMake module for hwloc detection
└── benchmark/
    ├── Makefile                  # Make-based build system
    ├── benchmark.hpp             # Benchmarking utilities
    ├── fib_serial.hpp            # Serial fibonacci
    ├── fib_nativeforkjoin.hpp    # Parallel fibonacci with TaskPaRTS
    ├── test_simple.cpp           # Simple test program
    └── [26+ benchmark programs]  # Various algorithm implementations
```

## Architecture

### Core Components

#### 1. Task Scheduler
- **Work-stealing**: Efficient load balancing across worker threads
- **Elastic scheduling**: Dynamic adaptation to workload (can be disabled)
- **Native continuations**: Low-level context switching for tasks

#### 2. Threading Model
- Uses `std::thread` for worker threads
- `std::condition_variable` for synchronization
- `std::semaphore` (C++20 style with semaphore.h)
- `std::atomic` for lock-free coordination
- Requires pthread library

#### 3. Memory Management
- Stack allocation for task contexts
- Optional jemalloc integration for better performance
- Cache-line aware (64-byte cache lines)
- CPU context: 64 bytes (X64) or 168 bytes (ARM64)

### Public API

**Header**: `include/taskparts/taskparts.hpp`

**Core Functions**:
```cpp
namespace taskparts {
  // Fork-join parallelism with two branches
  template <typename F1, typename F2>
  auto fork2join(F1&& f1, F2&& f2) -> void;

  // Get number of worker threads
  extern auto get_nb_workers() -> size_t;

  // Get current worker thread ID
  extern auto get_my_id() -> size_t;
}
```

### Header-Only Mode

**Trigger**: Define `TASKPARTS_HEADER_ONLY` before including headers

**Mechanism**: `include/taskparts/taskparts_internal.hpp:586-588`
```cpp
#ifdef TASKPARTS_HEADER_ONLY
#include "../../src/taskparts.cpp"
#endif
```

**Important**: When compiling in header-only mode:
- Define `-DTASKPARTS_HEADER_ONLY` (NOT `_LIBRARY`)
- Include both `include/` and `src/` directories in include path
- The implementation is automatically included via the header

## Build Systems

### 1. Nix Flake (Recommended)

**Entry Point**: `flake.nix`

**Features**:
- Reproducible development environment
- Integrated ParlayLib from https://github.com/mikerainey/parlaylib
- Automatic environment variable configuration
- All dependencies included (LLVM 18, hwloc, jemalloc, gdb, valgrind)

**Usage**:
```bash
# Enter development shell
nix develop --extra-experimental-features 'nix-command flakes'

# Or with experimental features enabled in nix.conf
nix develop
```

**Environment Variables Set**:
- `NUM_SYSTEM_CORES`: Detected via hwloc-ls
- `TASKPARTS_NUM_WORKERS`: Set to core count
- `PARLAY_NUM_THREADS`: Set to core count
- `MAKEFLAGS`: Parallel build jobs (-j $NUM_SYSTEM_CORES)
- `HWLOC_CFLAGS`: hwloc compiler flags
- `HWLOC_LIBFLAGS`: hwloc linker flags
- `PARLAYLIB_CFLAGS`: ParlayLib include paths
- `TASKPARTS_PLATFORM_FLAGS`: Platform/architecture flags
- `LD_PRELOAD`: jemalloc preload for benchmarks

### 2. CMake

**Minimum Version**: 3.14

**Build Modes**:

**Header-Only** (recommended):
```bash
cmake -S . -B build -DHWLOC=ON
cmake --build build
```

**Linked Library**:
```bash
cmake -S . -B build -DSHARED_LINKED_LIBRARY=ON -DHWLOC=ON
cmake --build build
```

**CMake Options**:
- `SHARED_LINKED_LIBRARY=ON`: Build as shared library instead of header-only
- `NONELASTIC=ON`: Disable elastic scheduling
- `STATS=ON`: Enable statistics collection
- `LOGGING=ON`: Enable logging
- `CHASELEV_DEQUE=ON`: Use Chase-Lev deque (requires NONELASTIC=ON)
- `HWLOC=ON`: Enable CPU pinning via hwloc

### 3. Make (Benchmarks)

**Location**: `benchmark/Makefile`

**Build Variants**:
- `.header_opt`: Header-only optimized (-O3 -march=native -DNDEBUG)
- `.header_sta`: Header-only with stats
- `.header_log`: Header-only with logging
- `.header_dbg`: Header-only debug (-O -g3)
- `.opt/.sta/.log/.dbg/.msdbg`: Linked library variants
- `.serial_opt/.serial_dbg`: Sequential (PARLAY_SEQUENTIAL)
- `.homegrown_opt/.homegrown_dbg`: Parlay scheduler
- `.opencilk_sta`: OpenCilk scheduler

**Example**:
```bash
cd benchmark
make fib.header_opt
./bin/fib.header_opt 35
```

## Compiler Flags

### Platform (Required - Pick One)
- `TASKPARTS_POSIX`: Linux and other POSIX systems
- `TASKPARTS_DARWIN`: macOS

### Architecture (Required - Pick One)
- `TASKPARTS_X64`: x86-64 architecture
- `TASKPARTS_ARM64`: ARM64 architecture

### Optional Features
- `TASKPARTS_USE_HWLOC`: Enable CPU pinning and topology awareness
- `TASKPARTS_DISABLE_ELASTIC`: Disable elastic scheduling
- `TASKPARTS_STATS`: Enable statistics collection
- `TASKPARTS_LOGGING`: Enable detailed logging
- `TASKPARTS_USE_CHASELEV_DEQUE`: Use alternative deque implementation
- `TASKPARTS_USE_VALGRIND`: Valgrind integration for memory debugging
- `TASKPARTS_RUN_UNIT_TESTS`: Enable unit tests
- `TASKPARTS_META_SCHEDULER_SERIAL_RANDOM`: Diagnostic mode

### Parlay Integration
- `PARLAY_TASKPARTS`: Use TaskPaRTS as Parlay's scheduler
- `PARLAY_SEQUENTIAL`: Sequential execution (no parallelism)
- `PARLAY_HOMEGROWN`: Use Parlay's homegrown scheduler
- `PARLAY_OPENCILK`: Use OpenCilk scheduler

## Dependencies

### Required
- C++17 compiler (GCC or Clang)
- CMake 3.14+ (for CMake builds)
- Make (for benchmark builds)
- libatomic (linking: `-latomic`)
- pthread (implicit via std::thread)

### Optional
- **hwloc**: CPU pinning and topology detection
  - Compile: `-DTASKPARTS_USE_HWLOC -I${hwloc.dev}/include/`
  - Link: `-L${hwloc.lib}/lib/ -lhwloc`
- **ParlayLib**: Parallel algorithms library (header-only)
  - Source: https://github.com/mikerainey/parlaylib (specific fork for TaskPaRTS)
  - Commit: 9bb91b2613f2644a879242215b0d525813c5131b
  - Include: `-I${parlaylib}/include -I${parlaylib}/share/examples`
- **jemalloc**: Memory allocator (runtime preload for benchmarks)
  - Runtime: `LD_PRELOAD=${jemalloc}/lib/libjemalloc.so`
- **valgrind**: Memory debugging
- **gdb**: Debugger

## Key Files

### Core Implementation

#### `include/taskparts/taskparts.hpp`
- Public API header (269 bytes)
- Includes `taskparts_internal.hpp`
- Defines namespace and public functions
- Entry point for users

#### `include/taskparts/taskparts_internal.hpp`
- Implementation details (15KB)
- Contains all template implementations
- Includes `src/taskparts.cpp` when `TASKPARTS_HEADER_ONLY` is defined (line 586-588)
- Defines work-stealing scheduler, continuations, threading primitives

#### `src/taskparts.cpp`
- Single compilation unit (79KB)
- Contains non-template implementations
- **Important Fix**: Added `#include <algorithm>` at line 17 for `std::stable_sort`
- Used for `stable_sort` in logging report function (line 1804)

### Benchmark Framework

#### `benchmark/benchmark.hpp`
- Requires ParlayLib: `#include <parlay/primitives.h>`
- Provides timing utilities, benchmarking harness
- Used by all benchmark programs

#### `benchmark/Makefile`
- **Important Fixes Applied**:
  - Line 9: Added `-I$(TASKPARTS_ROOT_PATH)/src/` to `TASKPARTS_CPP_FLAGS`
  - Lines 64-70: Changed `-DTASKPARTS_HEADER_ONLY_LIBRARY` → `-DTASKPARTS_HEADER_ONLY`
- Defines build rules for all benchmark variants
- Uses environment variables from Nix flake or manual setup

### Nix Configuration

#### `flake.nix`
- Main entry point for Nix users
- Defines development shell with all dependencies
- Builds ParlayLib from `nix/parlaylib.nix`
- Auto-detects platform (POSIX/Darwin) and architecture (X64/ARM64)
- Sets up comprehensive environment variables

#### `nix/parlaylib.nix`
- ParlayLib package definition
- Fetches from mikerainey/parlaylib fork
- Supports TaskPaRTS integration via `PARLAY_TASKPARTS` flag

## Known Issues and Fixes

### Issue 1: Missing `<algorithm>` Include
**File**: `src/taskparts.cpp`
**Line**: 1804
**Error**: `undefined reference to std::stable_sort`
**Fix**: Added `#include <algorithm>` at line 17
**Status**: ✅ Fixed

### Issue 2: Wrong Header-Only Flag
**File**: `benchmark/Makefile`
**Lines**: 64-70
**Error**: Used `-DTASKPARTS_HEADER_ONLY_LIBRARY` but header checks for `TASKPARTS_HEADER_ONLY`
**Fix**: Changed all occurrences to `-DTASKPARTS_HEADER_ONLY`
**Status**: ✅ Fixed

### Issue 3: Missing src/ Include Path
**File**: `benchmark/Makefile`
**Line**: 9
**Error**: Header-only mode couldn't find `../../src/taskparts.cpp` include
**Fix**: Added `-I$(TASKPARTS_ROOT_PATH)/src/` to `TASKPARTS_CPP_FLAGS`
**Status**: ✅ Fixed

### Issue 4: Outdated Nix Packages
**File**: `benchmark/shell.nix` (now deleted)
**Error**: `llvmPackages_14` removed from nixpkgs as obsolete
**Fix**: Created modern flake.nix with `llvmPackages_18`
**Status**: ✅ Fixed - Migrated to flake

## Development Workflow

### Setting Up Development Environment

#### Using Nix Flake (Recommended)
```bash
cd /path/to/taskparts
nix develop --extra-experimental-features 'nix-command flakes'
```

All environment variables and dependencies are automatically configured.

#### Manual Setup
Ensure these are available:
- C++17 compiler (g++ or clang++)
- CMake 3.14+
- hwloc (optional but recommended)
- ParlayLib (required for benchmarks)

Set environment variables:
```bash
export TASKPARTS_PLATFORM_FLAGS="-DTASKPARTS_POSIX=1 -DTASKPARTS_X64=1"
export HWLOC_CFLAGS="-DTASKPARTS_USE_HWLOC -I/path/to/hwloc/include"
export HWLOC_LIBFLAGS="-L/path/to/hwloc/lib -lhwloc"
export PARLAYLIB_CFLAGS="-I/path/to/parlaylib/include"
```

### Building the Library

#### Header-Only (CMake)
```bash
cmake -S . -B build -DHWLOC=ON
cmake --build build
```

#### Linked Library (CMake)
```bash
cmake -S . -B build -DSHARED_LINKED_LIBRARY=ON -DHWLOC=ON
cmake --build build
```

### Building Benchmarks

```bash
cd benchmark
make fib.header_opt          # Optimized fibonacci
make mergesort.header_opt    # Merge sort
make quickhull.header_opt    # Convex hull
```

### Running Benchmarks

```bash
./bin/fib.header_opt 40                    # Fibonacci of 40
FIB_THRESHOLD=10 ./bin/fib.header_opt 40   # With custom threshold
TASKPARTS_NUM_WORKERS=8 ./bin/fib.header_opt 40  # With 8 workers
```

### Creating a Simple Test Program

```cpp
#include <taskparts/taskparts.hpp>
#include <iostream>

long fib(long n) {
  if (n < 2) return n;
  long a, b;
  taskparts::fork2join([&] {
    a = fib(n - 1);
  }, [&] {
    b = fib(n - 2);
  });
  return a + b;
}

int main() {
  std::cout << "Workers: " << taskparts::get_nb_workers() << "\n";
  std::cout << "fib(30) = " << fib(30) << "\n";
  return 0;
}
```

**Compile**:
```bash
clang++ -std=c++17 -I../include/ -I../src/ \
  -DTASKPARTS_POSIX=1 -DTASKPARTS_X64=1 \
  -DTASKPARTS_HEADER_ONLY \
  -O3 -DNDEBUG \
  test.cpp -o test \
  -latomic -lpthread
```

## Testing

### Unit Tests
Enable with `-DTASKPARTS_RUN_UNIT_TESTS=1` flag.

### Benchmark Suite
26+ benchmark programs covering:
- **Basic algorithms**: fibonacci, merge sort, sample sort, quicksort
- **Computational geometry**: convex hull (quickhull), Delaunay triangulation
- **Graph algorithms**: BFS, Kruskal's MST, graph coloring, triangle counting
- **String algorithms**: suffix array, Rabin-Karp, LCP
- **Scientific computing**: N-body FMM, ray tracing, oscillator simulation
- **Data structures**: hash map, lock-free list, filter, tree reduction

### Verification

Test that the development environment works:
```bash
# Check environment variables
echo $NUM_SYSTEM_CORES
echo $TASKPARTS_NUM_WORKERS

# Build simple test
cd benchmark
make test_simple.header_opt
./bin/test_simple 30

# Build and run fibonacci
make fib.header_opt
./bin/fib.header_opt 35
```

Expected output: `fib(35) = 9227465` with execution time and worker count.

## Performance Considerations

### Compiler Optimizations
- `-O3`: Maximum optimization
- `-march=native`: Use CPU-specific instructions (disabled in Nix sandbox with warning)
- `-DNDEBUG`: Disable assertions
- `-fno-stack-protector`: Reduce overhead
- `-fno-asynchronous-unwind-tables`: Smaller binaries

### Runtime Configuration
- `TASKPARTS_NUM_WORKERS`: Set worker thread count (default: system cores)
- `FIB_THRESHOLD`: Fibonacci cutoff for sequential execution (default: see code)
- `TASKPARTS_BENCHMARK_WARMUP_SECS`: Warmup time before measurement
- `LD_PRELOAD=libjemalloc.so`: Use jemalloc for better allocation performance

### Build Mode Selection
- **Header-only**: Best performance (inlining, LTO opportunities)
- **Linked library**: Faster compilation, easier debugging
- **Stats enabled**: Performance overhead for collecting statistics
- **Logging enabled**: Significant overhead, use only for debugging

## Common Tasks

### Adding a New Benchmark Program

1. Create `benchmark/myprogram.cpp`:
```cpp
#include "benchmark.hpp"
#include <taskparts/taskparts.hpp>

int main(int argc, char* argv[]) {
  taskparts::benchmark([&] {
    // Your parallel algorithm here
  });
  return 0;
}
```

2. Build:
```bash
cd benchmark
make myprogram.header_opt
```

### Debugging Parallel Code

Enable debug build with stats and logging:
```bash
make myprogram.header_dbg
TASKPARTS_STATS=1 TASKPARTS_LOGGING=1 ./bin/myprogram.header_dbg
```

Or use GDB:
```bash
gdb ./bin/myprogram.header_dbg
```

Valgrind for memory issues:
```bash
make myprogram.header_dbg
valgrind --leak-check=full ./bin/myprogram.header_dbg
```

### Switching Schedulers

**TaskPaRTS** (default):
```bash
make myprogram.header_opt
```

**Sequential** (no parallelism):
```bash
make myprogram.serial_opt
```

**Parlay's homegrown scheduler**:
```bash
make myprogram.homegrown_opt
```

## Git Repository Info

**Current Branch**: `successor`
**Main Branch**: `main`
**Recent Work**: Nix flake modernization, header-only build fixes, parlaylib integration

**Branch History** (recent commits on successor):
- Nix flake with integrated parlaylib support
- Header-only build improvements
- Compile-time parameter for deque selection
- OpenCilk support
- Work-stealing optimizations

## Additional Resources

- **CMake Documentation**: See CMakeLists.txt for all available options
- **Development Notes**: DEVELOPMENT.md contains TODOs and known issues
- **License**: MIT (see LICENSE file)
- **ParlayLib**: https://cmuparlay.github.io/parlaylib/
- **Nix Flakes**: https://nixos.wiki/wiki/Flakes

## Tips for AI Assistants

1. **Always read before editing**: Use the Read tool to understand file contents before making changes
2. **Header-only mode**: Remember to include both `include/` and `src/` directories
3. **Correct flag**: Use `-DTASKPARTS_HEADER_ONLY`, not `_LIBRARY`
4. **Nix environment**: All benchmarks should be built within `nix develop` for proper dependencies
5. **Platform detection**: Auto-detect platform/architecture from context or ask user
6. **Benchmark dependencies**: Most benchmarks require ParlayLib - ensure it's available
7. **Test after changes**: Always verify builds work after modifying Makefiles or headers
8. **Git tracking**: Nix flakes require files to be git-tracked to work (`git add` before testing)

## Version Information

**Last Updated**: 2026-01-30
**TaskPaRTS Version**: 0.1.0 (from flake.nix)
**LLVM Version**: 18.1.8 (from Nix flake)
**CMake Version**: 4.1.2 (from Nix flake)
**ParlayLib Commit**: 9bb91b2613f2644a879242215b0d525813c5131b

---

This document is maintained to help AI assistants quickly understand and work effectively with the TaskPaRTS codebase. Update it when significant architectural or tooling changes occur.
