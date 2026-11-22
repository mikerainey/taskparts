#!/bin/sh
# Runs every benchmark of this directory, on small inputs, in the three
# builds for testing (see Makefile), on several numbers of workers. Each run
# compares its result with that of the serial version of the program. Prints
# one line per failure, and a count.
#
#   ./test.sh          a few minutes
#   ./test.sh quick    the build with pauses only
#
# With TBB=1 in the environment, the programs are built with their versions
# on oneTBB (see Makefile), and those are checked too, in the build with
# pauses only: oneTBB itself is not built for the sanitizers.

cd "$(dirname "$0")" || exit 1
builds="perturb asan tsan"
if [ "$1" = "quick" ]; then
  builds="perturb"
fi

programs="pipefib ferret x264 heat life lcs cholesky preorder"
for b in $builds; do
  for p in $programs; do
    make -s ${TBB:+TBB=1} "bin/$p.$b" || exit 1
  done
done

export TSAN_OPTIONS=halt_on_error=1
export ASAN_OPTIONS=detect_leaks=0
nb_runs=0
nb_failed=0

# check PROGRAM ARGS...: the run must succeed and print "check ok".
check() {
  nb_runs=$((nb_runs + 1))
  out=$("$@" 2>&1)
  status=$?
  if [ $status -ne 0 ] || ! printf '%s\n' "$out" | grep -q "^check *ok"; then
    nb_failed=$((nb_failed + 1))
    echo "FAILED (status $status, $TASKPARTS_NUM_WORKERS workers): $*"
    printf '%s\n' "$out" | tail -5
  fi
}

for b in $builds; do
  # The versions of a program that have the same arguments as its dag version.
  tbb=""
  if [ -n "$TBB" ] && [ "$b" = "perturb" ]; then
    tbb="tbb"
  fi
  for workers in 1 2 3 4 10 16; do
    export TASKPARTS_NUM_WORKERS=$workers
    # n, bits per stage, throttling limit (0: the default).
    for a in "2 1 0" "3 1 1" "300 1 0" "300 1 1" "300 1 2" "500 7 3" "2000 256 0"; do
      # shellcheck disable=SC2086
      check "bin/pipefib.$b" dag $a check
    done
    # Queries, entries of the database, throttling limit.
    for a in "0 50 0" "1 50 0" "200 300 0" "200 300 1" "200 5 2"; do
      for version in dag $tbb; do
        # shellcheck disable=SC2086
        check "bin/ferret.$b" $version $a check
      done
    done
    # Groups, width, height, in-between frames, throttling limit.
    for a in "0 64 64 2 0" "1 64 64 2 0" "12 64 96 2 0" "12 64 96 0 1" \
             "12 96 64 3 2" "8 32 16 1 0" "6 64 160 1 0"; do
      # shellcheck disable=SC2086
      check "bin/x264.$b" dag $a check
    done
    # n, time steps, narrowest side that is cut (halved), most steps that
    # are not cut.
    for a in "1 1 1 1" "7 9 1 1" "40 30 2 1" "64 40 4 2" "100 25 8 3" \
             "150 7 16 8"; do
      for version in ploops trap dag $tbb; do
        # shellcheck disable=SC2086
        check "bin/heat.$b" $version $a check
        # shellcheck disable=SC2086
        check "bin/life.$b" $version $a check
      done
    done
    # n, block size.
    for a in "1 1" "8 1" "50 7" "200 16" "300 300" "257 32"; do
      for version in planes dc dag $tbb; do
        # shellcheck disable=SC2086
        check "bin/lcs.$b" $version $a check
      done
    done
    # n, tile size.
    for a in "1 1" "8 1" "16 16" "48 4" "96 8" "100 25"; do
      for version in levels dag $tbb; do
        # shellcheck disable=SC2086
        check "bin/cholesky.$b" $version $a check
      done
    done
    # Cells.
    for a in 1 5 100 5000; do
      for version in levels dag $tbb; do
        check "bin/preorder.$b" $version "$a" check
      done
    done
  done
done

echo "$nb_runs runs, $nb_failed failed"
[ "$nb_failed" -eq 0 ]
