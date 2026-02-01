{
  description = "TaskPaRTS - A Task-Parallel Run-Time System for C++";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};

        # Detect platform and architecture
        platformFlags =
          if pkgs.stdenv.isDarwin then
            "-DTASKPARTS_DARWIN=1"
          else
            "-DTASKPARTS_POSIX=1";

        archFlags =
          if pkgs.stdenv.isAarch64 then
            "-DTASKPARTS_ARM64=1"
          else
            "-DTASKPARTS_X64=1";

        # Build parlaylib with taskparts integration
        # We pass taskparts as null initially since parlaylib doesn't strictly need it
        # for building, but it enables PARLAY_TASKPARTS mode when building examples
        parlaylib = pkgs.callPackage ./nix/parlaylib.nix {
          taskparts = null;
        };

        # TaskPaRTS package (header-only library)
        taskparts = pkgs.stdenv.mkDerivation {
          pname = "taskparts";
          version = "0.1.0";

          src = ./.;

          nativeBuildInputs = [ pkgs.cmake ];

          buildInputs = [ pkgs.hwloc ];

          cmakeFlags = [
            "-DHWLOC=ON"
          ];

          # Header-only library doesn't produce binaries
          # The build process just validates and installs headers
          installPhase = ''
            mkdir -p $out/include
            mkdir -p $out/src
            cp -r ${./include}/* $out/include/
            cp ${./src}/taskparts.cpp $out/src/
          '';

          meta = with pkgs.lib; {
            description = "A Task-Parallel Run-Time System for C++";
            license = licenses.mit;
            platforms = platforms.unix;
          };
        };

      in
      {
        packages = {
          default = taskparts;
          inherit parlaylib;
        };

        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            # Compiler toolchain (LLVM 18)
            llvmPackages_18.stdenv.cc

            # Build tools
            cmake
            gnumake

            # Required libraries
            hwloc
            jemalloc

            # Development and debugging tools
            gdb
            valgrind
            llvmPackages_18.clang-tools  # For clang-format

            # Parlaylib for benchmarks
            parlaylib
          ];

          shellHook = ''
            # Detect system cores via hwloc
            export NUM_SYSTEM_CORES=$(${pkgs.hwloc}/bin/hwloc-ls | grep Core | wc -l 2>/dev/null || echo 4)

            # Set worker counts
            export TASKPARTS_NUM_WORKERS=$NUM_SYSTEM_CORES
            export PARLAY_NUM_THREADS=$NUM_SYSTEM_CORES
            export MAKEFLAGS="-j $NUM_SYSTEM_CORES"

            # Library paths
            export LD_LIBRARY_PATH="$PWD/benchmark/bin:''${LD_LIBRARY_PATH:-}"

            # Benchmark configuration
            export TASKPARTS_BENCHMARK_WARMUP_SECS=0

            # Compiler/linker flags for Makefile
            export HWLOC_CFLAGS="-DTASKPARTS_USE_HWLOC -I${pkgs.hwloc.dev}/include/"
            export HWLOC_LIBFLAGS="-L${pkgs.hwloc.lib}/lib/ -lhwloc"
            export PARLAYLIB_CFLAGS="-I${parlaylib}/include -I${parlaylib}/share/examples"
            export TASKPARTS_OPTIONAL_FLAGS="$HWLOC_CFLAGS $PARLAYLIB_CFLAGS"
            export TASKPARTS_OPTIONAL_LINKER_FLAGS="$HWLOC_LIBFLAGS"

            # Platform and architecture flags
            export TASKPARTS_PLATFORM_FLAGS="${platformFlags} ${archFlags}"

            # Debug flags for development
            export TASKPARTS_DEBUG_CFLAGS="-DTASKPARTS_RUN_UNIT_TESTS=1 -DTASKPARTS_USE_VALGRIND=1"

            # Jemalloc configuration
            export JEMALLOC_PATH=${pkgs.jemalloc}
            export LD_PRELOAD=${pkgs.jemalloc}/lib/libjemalloc.so

            # Set CMDLINE_CFLAGS as empty (optional dependency not included)
            export CMDLINE_CFLAGS=""

            # OpenCilk placeholder (not included by default)
            export OPENCILK_CXX=""

            # Formatting functions
            fmt() {
              # prefer git list; fall back to find
              files="$(git ls-files '*.c' '*.h' '*.cc' '*.hh' '*.cpp' '*.hpp' '*.cxx' '*.hxx' 2>/dev/null || true)"
              if [ -z "$files" ]; then
                files="$(find . -type f \( -name '*.c' -o -name '*.h' -o -name '*.cc' -o -name '*.hh' -o -name '*.cpp' -o -name '*.hpp' -o -name '*.cxx' -o -name '*.hxx' \))"
              fi
              [ -z "$files" ] && { echo "No C/C++ files to format."; return 0; }
              echo "$files" | xargs -r ${pkgs.llvmPackages_18.clang-tools}/bin/clang-format -i
            }

            fmt-check() {
              set -e
              tmpdir="$(mktemp -d)"; trap 'rm -rf "$tmpdir"' EXIT
              changed=0
              for f in $(git ls-files '*.c' '*.h' '*.cc' '*.hh' '*.cpp' '*.hpp' '*.cxx' '*.hxx' 2>/dev/null || true); do
                cp "$f" "$tmpdir/file"
                ${pkgs.llvmPackages_18.clang-tools}/bin/clang-format -i "$tmpdir/file"
                if ! diff -u "$f" "$tmpdir/file" >/dev/null; then
                  echo "Needs format: $f"
                  changed=1
                fi
              done
              [ "$changed" -eq 0 ] && echo "clang-format: OK"
              exit "$changed"
            }

            echo "════════════════════════════════════════════════════════"
            echo "TaskPaRTS development environment loaded"
            echo "════════════════════════════════════════════════════════"
            echo "Cores detected:    $NUM_SYSTEM_CORES"
            echo "Worker threads:    $TASKPARTS_NUM_WORKERS"
            echo "Platform flags:    $TASKPARTS_PLATFORM_FLAGS"
            echo ""
            echo "Available commands:"
            echo "  cd benchmark && make <program>.header_opt    # Build optimized benchmark"
            echo "  cmake -S . -B build -DHWLOC=ON               # Configure CMake build"
            echo "  cmake --build build                          # Build with CMake"
            echo "  fmt                                          # Format all C/C++ files with clang-format"
            echo "  fmt-check                                    # Check if files need formatting (CI mode)"
            echo ""
            echo "Parlaylib included at: ${parlaylib}"
            echo "════════════════════════════════════════════════════════"
          '';
        };
      }
    );
}
