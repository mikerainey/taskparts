# Measurements for the paper

The tables, plots and examples of `../main.tex` cite values that Flexibench generated from
the measurements recorded in eight stores: four rounds on each of two machines, all
measured on 2026-11-21: the fourth in the night, and the first three around noon.
A ninth store, round 5 below, is from a third machine, which the paper calls the Xeon,
measured on 2026-11-22.

| | laptop | gamora |
|---|---|---|
| name in the paper | the M5 | the Ryzen |
| machine | Apple M5, 4 performance and 6 efficiency cores, macOS | AMD Ryzen 9 3950X, 16 cores, 32 hardware threads, Linux 6.12 |
| compiler | Apple clang 21.0.0 (clang-2100.4.1.1) | Clang 21.1.8 |
| Flexibench | 27e6e1e | 27e6e1e |
| stores | this directory, `bench/`, `curves/` | `gamora/`, `gamora/bench/`, `gamora/curves/` |
| view names | `times`, `bench`, `ctime`, ... | the same with `gamora` appended |
| workers | 1, 2, 4, 6, 8, 10 (10 is all its cores); curves on 1 to 10 | 1, 2, 4, 6, ..., 16 (16 is all its cores) and 32; curves on the same and 24 |

Lean is 4.30.0 on both. All the builds of rounds 1 to 3, on both machines, are from one
commit of taskparts on a local side branch: `8b54c6a7` (`perf-instr9`). It is `successor`
at 59452af7 plus this paper's micro taskparts as it is in this directory's commit: the
release-ordered store in `push`, stacks that each worker keeps for itself (up to 16) before
the shared pool, the array of packed counters in `fib.cpp` aligned to a cache line, and
deque slots that are not initialized. Round 4 was built from its parent, `90db65dc`
(`perf-instr8`), which differs in one place: when the program exits, `8b54c6a7` releases
the workers' stacks only if the workers were ever started. Without that, a program that
never forks crashes at exit, as the build `serial` of `fib` did; no program of round 4 is
of that kind.

On the laptop, each round waited for four consecutive samples at 93% idle or better, on AC
power. On gamora, each round ran in benchmark mode (governor `performance` on all 32 CPUs,
before and after; boost on; SMT on). One job ran at a time on each machine.

In every round a time is the time of a region: the call of `fib`, or one run of a
program's algorithm, between two snapshots. The starting of the workers' threads and the
generation of inputs are outside every region.

### Why the packed counters are aligned

In an earlier `fib` round, never committed here, the build `packed` took 0.25 s on 4
workers on the laptop, against 1.46 s in the round before it. The cause was where the
array of counters landed: two new variables had moved it, so that worker 0's counter was
alone on one cache line and the others on the next. Building the earlier source with the
same compiler gave 1.46 s again, and so did the new source with the array aligned. So the
array is aligned, and the counters of workers 0 to 7 always share a line.

## Round 1: `fib`, in this directory

`.flexibench/` holds:
- `fib 38` with seven builds of the instrumented micro taskparts, on 1, 2, 4, 6, 8 and 10
  workers (gamora: 1, 2, 4, 6, 8, 10, 12, 14, 16 and 32), with five runs of each and the runs of the seven builds
  interleaved;
- the stats line that each instrumented run printed;
- correctness checks of every build, on 3 and 16 workers (gamora: 3, 24 and 64), all of
  which passed: `fib 27`; `migrate`, for the
  five builds that have it; and, for the three builds that count forks, that `fib 27`
  counts exactly 317810.

| View | What it shows | Certificate |
|---|---|---|
| `times` | median time of each build at each worker count, to the millisecond (Table `tbl:cost`) | `afbc636dc71f6d474633c54f8aa635115c3f79e87d71708136106901645cc6b9` |
| `work`, `sched`, `idle`, `steals` | medians of the totals that the build `stats` reported (the example of Appendix `sec:collect`) | `bf1e79d83970d8c51dd4b0e9024699340f66f2b1e71233478176cf084c088ff5` |
| `timesgamora` | the same on gamora (Table `tbl:costgamora`) | `70bcd32b9b2c78071b2a25f9f5ee8da78ae9e5dfd47f0a97df9e5df490587d08` |
| `workgamora`, `schedgamora`, `idlegamora`, `stealsgamora` | the totals on gamora | `4f51f80b895e53ed5710a795441921292f4e04da7d1161a7085627880c0aff2f` |

Each view is a `.tex` and an `.html` file of its name. After `\input{perf/times}`,
`\fbval{times}{stats}{P = 4}` prints a value. `times.bib` is the paper's citation,
`flexibench-times`, written by hand for now.

The builds are of `doc/instrumentation/microtaskparts`, with `-O2 -falign-functions=64`.

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

## Round 2: benchmark programs, in `bench/`

`bench/.flexibench/` holds mergesort (20 million elements), samplesort (50 million),
quickhull (50 million points) and BFS (2 million vertices). The drivers are
`benchmark/{mergesort,samplesort,quickhull,BFS}.cpp` of this repository, unmodified, built
against the harness `doc/instrumentation/benchmark/benchmark.hpp`. Their algorithms are
ParlayLib's examples at commit 80bf19d of https://github.com/mikerainey/parlaylib,
unmodified.

Each program ran with five builds: `serial` (ParlayLib with no scheduler), and micro
taskparts as `plain`, `stats`, `log` and `forks`, through `doc/instrumentation/parlay-adapter`,
on the worker counts of round 1. Each ran in five processes, with the runs
of the builds interleaved, and once more on 3 and 16 workers (gamora: 3 and 64) for its
result only. Every run printed the expected result. ParlayLib's generators give other
inputs on Linux than on macOS, so the expected results differ between the machines. Flags: `-O3 -DNDEBUG -falign-functions=64`.

A process runs its algorithm five times, each as a region. A value is the median over the
five processes of the median over a process's regions.

| View | What it shows | Certificate |
|---|---|---|
| `bench` | time of a region, for each program, build and worker count (Table `tbl:bench`) | `e8cb0e8b7afb9a4b5d6dd75c62101d291f9565fe73cd8aeb8b24193753b88c78` |
| `rtime`, `rwork`, `rsched`, `ridle`, `rsteals` | for the build `stats`: the time of a region, and the work, scheduling and idle time and the steals it reported (Table `tbl:factors`) | `4d7715637a2a189ce593198d664300d945055bfdd9b0f5b4f5b83d9c80cc582b` |
| `serial` | time of a region with the serial build (Table `tbl:factors`) | `79efc3d288bc7ded79f08ae06827e54f09670272d1d653f497ff95d75c10d495` |
| `benchgamora` | `bench` on gamora | `1fbbc0c488e307cc1d70210252151b473ea0854f21a3ede278f9f1f4f0c4c57a` |
| `rtimegamora`, `rworkgamora`, `rschedgamora`, `ridlegamora`, `rstealsgamora` | the totals on gamora (Table `tbl:factorsgamora`) | `3145dd67d244d438e72f6cbe35afec19bfdf8975e7ef0b987dc8ba5250dd0d09` |
| `serialgamora` | `serial` on gamora | `7c0e13a3d2f6bca9632e704261d694ebc33190fc539f6fbab140df9bdaf2ca28` |

`\fbval{bench}{mergesort stats}{P = 4}`, `\fbval{rwork}{mergesort}{P = 8}` and
`\fbval{serial}{mergesort}{serial}` print values. `bench/bench.bib` is the citation
`flexibench-bench`.

## Round 3: the same programs on a range of worker counts, in `curves/`

`curves/.flexibench/` holds the four programs of round 2, from the same builds, with the
build `stats` on each number of workers from 1 to 10 (gamora: 1, 2, 4, 6, 8, 10, 12, 14,
16, 24 and 32) and with the build `serial`, in five
processes each, interleaved. Every run printed the expected result. Values are medians as
in round 2. Figures `fig:factored` and `fig:factoredgamora` are drawn from them:
`../plots.tex` computes every coordinate from these cells.

| View | What it shows | Certificate |
|---|---|---|
| `ctime`, `cwork`, `csched`, `cidle` | for the build `stats`: the time of a region, and the work, scheduling and idle time it reported | `4f8398f896259e7ef32a45efcaeb11a2e6d74698462a52ddfea43dc64dbd89c1` |
| `cserial` | time of a region with the serial build | `5f97f27aed996d70baffe73d8ceb8414f328fe05c1282bfb968e6ae65d29f541` |
| `ctimegamora`, `cworkgamora`, `cschedgamora`, `cidlegamora` | the same on gamora | `3efd68d0113ec6277a2ac466b3bea966ba34e6134971d90390b96f3b54ca6aae` |
| `cserialgamora` | `cserial` on gamora | `407d07a26432ebb85ef07ef158ecbdaff4b3af3bf438b5c2b5496beaca9a6e74` |

`curves/curves.bib` is the citation `flexibench-curves`. The client is `mtpinstrcurves`
(`client/MtpInstrCurves.lean`). Check it from `curves/` as
the others, with `flexibench check-view ctime.tex` and so on.

## Round 4: the ParlayLib examples as a whole, in `suite/` and `gamora/suite/`

Measured in the night of 2026-11-21, before the three rounds above; Tables `tbl:suite`
and `tbl:suitegamora` (Appendix `sec:suite`) are drawn from it. Each of the two stores
holds 60 of the 63 example programs of ParlayLib, from
https://github.com/mikerainey/parlaylib: 59 at commit 783e811, and `karatsuba` at commit
1e1d0b8, which keeps the program from reading a number beyond its end. Three builds:

| Row | What it is |
|---|---|
| `serial` | ParlayLib with no scheduler (`-DPARLAY_SEQUENTIAL`); the builds shared with the native fork join paper, `parlay-783e8116ec9d-serial-all` and `parlay-1e1d0b8db0c2-serial-karatsuba` |
| `plain` | micro taskparts with no instrumentation, through `../parlay-adapter` |
| `stats` | the same with `-DMTP_STATS` |

The builds `plain` and `stats` are from commit `90db65dc` of taskparts, on a local side
branch (`perf-instr8`): `successor` at 59452af7 plus `../parlay-adapter/timer-regions.patch`
and the micro taskparts with the workers' own stacks (see the top of this file). Flags: CMake `Release` and
`-falign-functions=64`. Each of the three build directories of the round joins the build
of the 59 programs and the build of `karatsuba` (`scripts/join-suite-instr`).

**The timer.** The example drivers time their algorithm with ParlayLib's timer
(`include/parlay/internal/get_time.h`). The patch, applied to a copy of ParlayLib when
`plain` and `stats` are built, makes the timer take a snapshot wherever it reads the clock
and print the totals of each timed interval. It does nothing unless `MTP_STATS` is
defined. The fork itself is not changed.

**Inputs and programs.** The sizes, the input files and the list of programs are those of
the native fork join paper's suite round (`../../native-fork-join/perf/README.md`), and so
are the three programs left out: `hash_map`, `decision_tree_c45` and `lasso_regression`.
`karatsuba` has no expected output, so only its success is checked.

**Runs.** Workers: 1, 4 and 10 on the laptop; 1, 16 and 32 on gamora. Each program and
build ran in three processes, interleaved, and once more on 3 and 16 workers (gamora: 3
and 64) for its output only. Every run printed the expected output: the `failures`
analysis is empty on both machines. The laptop's round took 1 h 34 min, after the usual wait for an idle
machine, and gamora's 2 h 49 min, in benchmark mode.

A value is the median over the three processes of the median over a process's regions.
The factors are computed by the analysis `factors` from those medians: `speedup` is the
time of `serial` over the time of `stats`; `forks` the time of `serial` over the work of
`stats` on one worker; `inflation` the work over the work on one worker; `utilization`
the work over the sum of work, scheduling and idle time. Its rows are keyed by `nworkers`
and not by the parameter `workers`, because a row also depends on runs with other
parameters, which Flexibench's check of keys rightly refuses under the name `workers`.

| View | What it shows | Certificate |
|---|---|---|
| `suite` | time of a region for each program, with `serial`, and with `plain` and `stats` at each worker count | `3f6dfc0293c037b7f694db7a65038a1f1965eaf40d7be26fb98b77b150436f1e` |
| `swork`, `ssched`, `sidle`, `ssteals` | the totals that `stats` reported | `7360602281a2dacd2abfe448932ff35e949600e673a4d5433160d072362b4205` |
| `sspeedup`, `sforks`, `sinflation`, `sutil` | the factors | `afc9c9c09cc3f23065b5f21892273bffcd5c23463539fc61b106150d03980c91` |
| (no view) | `failures`, which is empty | `66141cda238a907e4e39527c14ee2df091ff6af56d16230a83141368395d634d` |
| `suitegamora` | `suite` on gamora | `9eb305ffac05f8e3dccd0e65402dbbf0dd476cff0ddd1345ff8db2a3f3370747` |
| `sworkgamora`, `sschedgamora`, `sidlegamora`, `sstealsgamora` | the totals on gamora | `fe3186fde241db6e1504071b423a37c6d8c2b5ebb31e91700fea5dd0d818858e` |
| `sspeedupgamora`, `sforksgamora`, `sinflationgamora`, `sutilgamora` | the factors on gamora | `6c2134b64e8198dc17f9dc722fd7904ae46a45893177acb7018d6040a958817e` |
| (no view) | `failures` on gamora, which is empty | `11a72b7d935ddac890e67a15afd4adb7305e41c9cff8173181e5aa086a1beb15` |

`suite/suite.bib` and `gamora/suite/suitegamora.bib` are the citations `flexibench-suite`
and `flexibench-suitegamora`. The clients are `mtpinstrsuite` and `mtpinstrsuitegamora`
(`client/MtpInstrSuite*.lean`, generated by `suite/gen-client-instr.py`), built with
`scripts/build-suite-instr`; the sessions are `runs/suite2-*-build.sh` and
`runs/suite2-*-round.sh`, with their logs. Flexibench is 27e6e1e.

Each of these two stores is about 240 MB, because it holds the 180 executables of
the round. An earlier measurement of this round, on 2026-11-20 with the shared pool of stacks alone
and without `karatsuba`, is kept outside this repository, in `runs/laptop-suite` and
`runs/gamora-suite` of the taskparts-bench worktree. The counts and medians in the prose of Appendix `sec:suite` (how many programs
are within 2%, the median speedup, and so on) are derived from the views by hand; the
commands below don't check them.

## Round 5: the ParlayLib examples on 80 cores, in `icelake/suite/`

Measured on 2026-11-22 on a third machine, which the paper calls the Xeon: a rented
bare-metal node (Chameleon, `compute_icelake_r650`) with two Intel Xeon Platinum 8380,
40 cores each, 160 hardware threads, 125 GB, Ubuntu 24.04, Linux 6.8. The node no longer
exists; the directory's name, `icelake`, is the name the scripts gave it.

- **Builds.** micro taskparts `8b54c6a7` (`perf-instr9`), as rounds 1 to 3, and
  ParlayLib `1e1d0b8` for all 60 programs, with `timer-regions.patch`; clang 21.1.8,
  `-O3 -DNDEBUG -falign-functions=64`. The build with no scheduler is the companion
  paper's on that node (`PARLAY_SEQUENTIAL`, the same ParlayLib commit and compiler).
- **Sizes.** Those the native fork join paper chose for this machine
  (`suite/programs-icelake.txt` in the measurement repository): larger than the Ryzen's
  for about a dozen programs.
- **Settings.** Every process ran under `numactl --interleave=all`; no thread was
  pinned. Governor `performance`, automatic NUMA balancing off, turbo and SMT on.
- **Plan.** 1140 trials. The build with no scheduler and the one-worker runs of both
  builds of micro taskparts: one process each. The build with totals on 40, 80 and 160
  workers, and the build with no instrumentation on 80: three processes each,
  interleaved. Checks of the output of both builds on 3 and on 320 workers.
- **Client.** `mtpinstrsuiteicelake` (`client/MtpInstrSuiteIcelake.lean`, written by
  `suite/gen-client-instr.py` with `ONCE=1 PLAIN_WORKERS=80`), measurement repository
  commit `1911121`; session `runs/run-icelake.sh suite`.

| analysis | certificate |
|---|---|
| `times` (cited as `flexibench-suiteicelake`) | `2cfecefe4f5929d7b9fbc1ea63719e2b39aacb5bb6aaa4c86b6424410e9f86e0` |
| `totals` | `f01d9b25f687e5a11d7fadcc4b6177fc326da0041b17240a3dca02495fb1b415` |
| `factors` | `81ae4086532d7c1a99df9773765eedda5478870ada34e9d84a1cfe426d1832e9` |
| `failures` (empty) | `61f82f1a8d9bb4d3908b97d3d88755ecf79b19ca0b7fc0985e9f4ed61fadab39` |

The views `sidleicelake` and `sstealsicelake` were written on the laptop from the copied
store, with the same client built for macOS; the others on the node.

A round of `fib(46)` with the seven builds was also run on the node and is not in the
paper: the build with packed counters has room for 64 workers, so it failed on 80 and
160, and the analysis of times has no value for those cells.

## The clients

In the taskparts-bench repository, on its branch `gamora-instr` (local, not pushed):
`mtpinstr`, `mtpinstrbench` and `mtpinstrcurves` for the laptop, and `mtpinstrgamora`,
`mtpinstrbenchgamora` and `mtpinstrcurvesgamora` for gamora (`client/MtpInstr*.lean`),
with `scripts/build-mtp-instr` and `scripts/build-mtp-instr-bench`. All six were built
from that branch's flake, against Flexibench 27e6e1e; `runs/run-laptop-counts2.sh`,
`runs/run-laptop-bench4.sh` (round 2 on the laptop) and `runs/run-gamora-counts2.sh` there
are the sessions, with their logs. `gamora/gamora.bib` is
the citation `flexibench-gamora`.

## Earlier rounds

The rounds 1 to 3 of commit 59452af7 of this repository were measured on 2026-11-17 from
commit 4ff1190e (`perf-instr6`), with the shared pool of stacks alone. Measured again with
the workers' own stacks, they changed little: on gamora, one cell of the 322 in `timesgamora`,
`benchgamora`, `serialgamora`, `rtimegamora`, `ctimegamora` and `cserialgamora` moved by 5%
or more. On the laptop, the first attempt at round 2 (`runs/laptop-bench3`, not kept here)
was up to 17% slower than round 3, measured just after it from the same builds, `serial`
included; it was measured once more, and the round kept here agrees with round 3 within 2%.

Before those, the six rounds were run in the night of 2026-11-16 from commits 2d4c698b and
1a2f7aed (`perf-instr4`, `perf-instr5`), on fewer worker counts and with the deque slots
initialized. They were never committed here.

This directory's history holds the rounds before those:
- at commit 6797978, the three rounds on the laptop built from commit 5b6a1f7, before the
  release-ordered `push` and the pool of stacks;
- at commit f5307a8, `fib` and four ParlayLib example programs, built from commit f1f2775,
  in which each outermost call started its own threads, and a ParlayLib program ran as one
  outermost call;
- at commit 8080747, `fib` built from commit ea6814e, before a task lost its destructor.

## Checking

All nine stores check with Flexibench 27e6e1e. In each of
the nine directories that holds a `.flexibench/`:

    flexibench check-objects
    flexibench check <certificate>        # each certificate listed above for that store
    flexibench check-view <view>.tex      # each view, and its .html

These commands confirm three things:
- every object in a store has the digest it is stored under;
- each certificate holds for its store's log;
- each view shows exactly what its certificate certifies.

They don't check how the tables in the paper are laid out around the values, nor the
ratios that the paper's prose derives from them.

The three stores of rounds 4 and 5 (`suite/`, `gamora/suite/` and `icelake/suite/`, about
230 MB each) are too large for the repository, and `perf/.gitignore` leaves them out.
Their views, which the paper inputs, are in the repository, and `check-view` needs the
store beside them. Ask the author for a copy of a store.
