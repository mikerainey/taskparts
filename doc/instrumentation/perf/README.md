# Measurements for the paper

The tables and the example of `../main.tex` cite values that Flexibench generated from
the measurements recorded in two stores, one per round. Both rounds were measured on
2026-11-12 on the Apple M5 described in the paper, with Flexibench 2a10c0c and Lean 4.30.0.

## Round 1: `fib`, in this directory

`.flexibench/` holds:
- `fib 38` with seven builds of the instrumented micro taskparts, on 1, 4 and 8 workers,
  with five runs of each and the runs of the seven builds interleaved;
- the stats line that each instrumented run printed;
- correctness checks of every build, all of which passed: `fib 27`; `migrate`, for the
  five builds that have it; and, for the three builds that count forks, that `fib 27`
  counts exactly 317810.

| View | Files | What it shows | Certificate |
|---|---|---|---|
| `times` | `times.tex`, `times.html` | median time of each build at each worker count (Table `tbl:cost`) | `9e701cbd5d8aeac51059bb610c93bc894899a51a0de8f2183949a8561be2afc7` |
| `work`, `sched`, `idle`, `steals` | the `.tex` and `.html` of each name | medians of the totals that the build `stats` reported (the example of Appendix `sec:collect`) | `20782fa87bd0845407e137edabf167ba845d564b71aef6833005bbb6f7317804` |

After `\input{perf/times}`, `\fbval{times}{stats}{P = 4}` prints a value. `times.bib` is
the paper's citation, `flexibench-times`, written by hand for now.

All seven are builds of `doc/instrumentation/microtaskparts` at commit f1f2775, on the
branch `perf-instr2`, with `-O2 -falign-functions=64`.

| Row | Flag | What it is |
|---|---|---|
| `serial` | `-DMTP_SERIAL` | `fork2join` is two calls, with no scheduler |
| `plain` | none | no instrumentation |
| `stats` | `-DMTP_STATS` | time per worker in each state, and steals |
| `log` | `-DMTP_LOG` | `stats`, plus the log of changes of state and steals |
| `forks` | `-DMTP_COUNT_FORKS` | `stats`, plus a count of forks in each worker's recorder |
| `atomic` | `-DCOUNT_FORKS_ATOMIC` | `fib` only: `plain`, plus a count of forks in one shared atomic counter |
| `packed` | `-DCOUNT_FORKS_PACKED` | `fib` only: `plain`, plus a count of forks in per-worker counters packed in one array |

The `serial` build ignores the number of workers, and the paper's table shows it for one
worker only.

## Round 2: ParlayLib programs, in `parlay/`

`parlay/.flexibench/` holds mergesort (20 million elements), samplesort (50 million),
quickhull (50 million points) and BFS (2 million vertices), from ParlayLib's examples at
commit 80bf19d of https://github.com/mikerainey/parlaylib, unmodified. Each ran with four
builds (`plain`, `stats`, `log`, `forks`, as above) on 1, 4 and 8 workers, five times, with
the runs of the builds interleaved, and once more on 2 and 16 workers for its result only.
Every run printed the expected result.

The programs run on micro taskparts through `doc/parlay-adapter`, at the same commit
f1f2775, built with CMake's Release flags and `-falign-functions=64`. A time is the median
of the runs of the algorithm within one process, and a cell the median of that over the
five processes.

`parlay/parlay.tex` and `parlay/parlay.html` are the view `parlay` (Table `tbl:parlay`),
certified by `3e17012dabe6ec650eca0fd7e1391958547c1b08df09d82f568f643be45d8bed`.
`\fbval{parlay}{mergesort stats}{P = 4}` prints a value. `parlay/parlay.bib` is the
citation `flexibench-parlay`.

## The clients

`mtpinstr` and `mtpinstrparlay`, in the taskparts-bench repository (`client/MtpInstr.lean`,
`client/MtpInstrParlay.lean`, `scripts/build-mtp-instr`, `scripts/build-bench`,
`runs/run-tbl-instr2.sh`, `runs/run-tbl-instr-parlay.sh`).

## Earlier rounds

At commit 8080747, this directory held a round of six builds at commit ea6814e, before a
task lost its destructor: there, every return from `fork2join` ran it, and the build with
no instrumentation took 0.32 s on one worker.

## Checking

With Flexibench 2a10c0c, from this directory:

    flexibench check-objects
    flexibench check 9e701cbd5d8aeac51059bb610c93bc894899a51a0de8f2183949a8561be2afc7
    flexibench check 20782fa87bd0845407e137edabf167ba845d564b71aef6833005bbb6f7317804
    flexibench check-view times.tex     # and work, sched, idle, steals; and the .html

and from `parlay/`:

    flexibench check-objects
    flexibench check 3e17012dabe6ec650eca0fd7e1391958547c1b08df09d82f568f643be45d8bed
    flexibench check-view parlay.tex
    flexibench check-view parlay.html

These commands confirm three things:
- every object in a store has the digest it is stored under;
- each certificate holds for its store's log;
- each view shows exactly what its certificate certifies.

They don't check how the tables in the paper are laid out around the values, nor the
ratios that the paper's prose derives from them.
