#!/bin/sh
# Runs every test program in the three builds for testing (see Makefile), and
# in three more with all of the instrumentation on, on several numbers of
# workers. Prints one line per failure, and a count.
#
#   ./test.sh          about a minute
#   ./test.sh long     more rounds and seeds
#
# The mutation check, which says whether these tests can detect the errors
# they aim at, is separate: ./mutants.py

cd "$(dirname "$0")" || exit 1
rounds=2
seeds="1"
if [ "$1" = "long" ]; then
  rounds=10
  seeds="1 2 3 4 5"
fi

programs="fib migrate mixed stress seidel pipeline instr"
for b in perturb asan tsan iperturb itsan ievery; do
  for p in $programs; do
    make -s "bin/$p.$b" || exit 1
  done
done

export TSAN_OPTIONS=halt_on_error=1
export ASAN_OPTIONS=detect_leaks=0
export MTP_STRESS_TIMEOUT=60
nb_runs=0
nb_failed=0

# check EXPECTED-TEXT PROGRAM ARGS...: the run must succeed and print the text.
check() {
  expected=$1
  shift
  nb_runs=$((nb_runs + 1))
  out=$("$@" 2>&1)
  status=$?
  if [ $status -ne 0 ] || ! printf '%s\n' "$out" | grep -q "$expected"; then
    nb_failed=$((nb_failed + 1))
    echo "FAILED (status $status, $TASKPARTS_NUM_WORKERS workers): $*"
    printf '%s\n' "$out" | tail -5
  fi
}

# check_stops EXPECTED-TEXT PROGRAM ARGS...: the run must fail and print the text.
check_stops() {
  expected=$1
  shift
  nb_runs=$((nb_runs + 1))
  out=$("$@" 2>&1)
  status=$?
  if [ $status -eq 0 ] || ! printf '%s\n' "$out" | grep -q "$expected"; then
    nb_failed=$((nb_failed + 1))
    echo "FAILED TO STOP (status $status, $TASKPARTS_NUM_WORKERS workers): $*"
    printf '%s\n' "$out" | tail -5
  fi
}

for b in perturb asan tsan iperturb itsan ievery; do
  for workers in 1 2 3 4 10 16; do
    export TASKPARTS_NUM_WORKERS=$workers
    check_stops "nothing waited for" "bin/instr.$b" orphan
    check "fib(24) = 46368" "bin/fib.$b" 24
    check "ok" "bin/migrate.$b"
    check "^ok" "bin/instr.$b" "$rounds"
    check "round $((rounds - 1)): ok" "bin/mixed.$b" 22 "$rounds"
    for seed in $seeds; do
      check "^ok" "bin/stress.$b" "$rounds" "$seed"
    done
    # The stencil, on grids of one block, of one-cell blocks, and of blocks
    # that do not divide into many: n, block size, time steps.
    for grid in "1 1 9" "8 8 50" "8 1 30" "16 1 40" "64 2 20" "100 10 7"; do
      for version in planes dag; do
        # shellcheck disable=SC2086
        check "^check *ok" "bin/seidel.$b" $version $grid check
      done
    done
    # The pipeline: chunks, chunk bytes, throttling limit (0: the default).
    for stream in "0 64 0" "1 64 0" "500 256 0" "500 256 1" "500 256 2" "2000 64 7"; do
      # shellcheck disable=SC2086
      check "^check *ok" "bin/pipeline.$b" dag $stream check
    done
  done
done

# The trace of a build that logs is JSON, with a slice for some stay in a state.
make -s bin/instr.log || exit 1
trace=$(mktemp "${TMPDIR:-/tmp}/mtp-trace.XXXXXX")
export TASKPARTS_NUM_WORKERS=4
check "^ok" env MTP_TRACE="$trace" bin/instr.log 1
check "slices" python3 -c "
import json, sys
events = json.load(open(sys.argv[1]))['traceEvents']
slices = [e for e in events if e['ph'] == 'X']
assert slices and all(e['dur'] > 0 for e in slices)
print(len(slices), 'slices')" "$trace"
rm -f "$trace"

echo "$nb_runs runs, $nb_failed failed"
[ "$nb_failed" -eq 0 ]
