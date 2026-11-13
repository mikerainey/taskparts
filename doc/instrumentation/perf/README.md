# Measurements for the paper

The tables and the examples of `../main.tex` cite values that Flexibench generated from
the measurements recorded in two stores, one per round. Both rounds were measured on
2026-11-12 on the Apple M5 described in the paper, with Flexibench 2a10c0c and Lean 4.30.0,
and built from taskparts commit 5b6a1f7, on the branch `perf-instr3`.

In both rounds a time is the time of a region: the call of `fib`, or one run of a
program's algorithm, between two snapshots. The starting of the workers' threads and the
generation of inputs are outside every region.

## Round 1: `fib`, in this directory

`.flexibench/` holds:
- `fib 38` with seven builds of the instrumented micro taskparts, on 1, 4 and 8 workers,
  with five runs of each and the runs of the seven builds interleaved;
- the stats line that each instrumented run printed;
- correctness checks of every build, all of which passed: `fib 27`; `migrate`, for the
  five builds that have it; and, for the three builds that count forks, that `fib 27`
  counts exactly 317810.

| View | What it shows | Certificate |
|---|---|---|
| `times` | median time of each build at each worker count (Table `tbl:cost`) | `7f1c371dc24cd5ce17584f63a127a1625834f771aac3715e64422abf43b2f9df` |
| `work`, `sched`, `idle`, `steals` | medians of the totals that the build `stats` reported (the example of Appendix `sec:collect`) | `5b5b4c9b30dee13fb81ae242ed638ebe738c0b309d183055b821222cdeda2bb7` |

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
on 1, 4 and 8 workers. Each ran in five processes, with the runs of the builds interleaved,
and once more on 2 and 16 workers for its result only. Every run printed the expected
result. Flags: `-O3 -DNDEBUG -falign-functions=64`.

A process runs its algorithm five times, each as a region. A value is the median over the
five processes of the median over a process's regions.

| View | What it shows | Certificate |
|---|---|---|
| `bench` | time of a region, for each program, build and worker count (Table `tbl:bench`) | `666dea0fbcf8771431de860ca9047b270c6f3df051543bf68d112edcdafd936e` |
| `rtime`, `rwork`, `rsched`, `ridle`, `rsteals` | for the build `stats`: the time of a region, and the work, scheduling and idle time and the steals it reported (Table `tbl:factors`) | `b1374c1faab61eab8a983f530a3cf2efa8011be1e89cc610e3852d19f117d725` |
| `serial` | time of a region with the serial build (Table `tbl:factors`) | `59f8c892c7f8aaad77a198ee361fe5b36315993514ef61abd26ac558307745e6` |

`\fbval{bench}{mergesort stats}{P = 4}`, `\fbval{rwork}{mergesort}{P = 8}` and
`\fbval{serial}{mergesort}{serial}` print values. `bench/bench.bib` is the citation
`flexibench-bench`.

## Round 3: the same programs on 1 to 10 workers, in `curves/`

`curves/.flexibench/` holds the four programs of round 2, from the same builds, with the
build `stats` on each number of workers from 1 to 10 and with the build `serial`, in five
processes each, interleaved. Every run printed the expected result. Values are medians as
in round 2. Figure `fig:factored` is drawn from them: `../plots.tex` computes every
coordinate from these cells.

| View | What it shows | Certificate |
|---|---|---|
| `ctime`, `cwork`, `csched`, `cidle` | for the build `stats`: the time of a region, and the work, scheduling and idle time it reported | `6e4688ee62eca5cb7b6054aadb41ba2d55dbbcde59b784e851613d322b8a5a4d` |
| `cserial` | time of a region with the serial build | `9ddc39179f6b92a09faad1d0e8b077c2648cba6f20c25ffb4adfb26af9d0436c` |

`curves/curves.bib` is the citation `flexibench-curves`. The client is `mtpinstrcurves`
(`client/MtpInstrCurves.lean`, `runs/run-tbl-instr-curves.sh`). Check it from `curves/` as
the others, with `flexibench check-view ctime.tex` and so on.

## An earlier round, for one comparison: `relaunch/sched.tex`

`relaunch/sched.tex` is the view `sched` of an earlier round, in which each outermost call
started its own threads. The paper cites one value of it, the scheduling time of `fib 38`
on 8 workers. Its store and its certificate
(`20782fa87bd0845407e137edabf167ba845d564b71aef6833005bbb6f7317804`) are in this
directory at commit f5307a8, where `flexibench check-view sched.tex` checks it.

## The clients

`mtpinstr` and `mtpinstrbench`, in the taskparts-bench repository (`client/MtpInstr.lean`,
`client/MtpInstrBench.lean`, `scripts/build-mtp-instr`, `scripts/build-mtp-instr-bench`,
`runs/run-tbl-instr3.sh`, `runs/run-tbl-instr-bench.sh`).

## Earlier rounds

This directory's history holds the rounds this one replaced:
- at commit f5307a8, `fib` and four ParlayLib example programs, built from commit f1f2775,
  in which each outermost call started its own threads, and a ParlayLib program ran as one
  outermost call;
- at commit 8080747, `fib` built from commit ea6814e, before a task lost its destructor.

## Checking

With Flexibench 2a10c0c, from this directory:

    flexibench check-objects
    flexibench check 7f1c371dc24cd5ce17584f63a127a1625834f771aac3715e64422abf43b2f9df
    flexibench check 5b5b4c9b30dee13fb81ae242ed638ebe738c0b309d183055b821222cdeda2bb7
    flexibench check-view times.tex     # and work, sched, idle, steals; and the .html

and from `bench/`:

    flexibench check-objects
    flexibench check 666dea0fbcf8771431de860ca9047b270c6f3df051543bf68d112edcdafd936e
    flexibench check b1374c1faab61eab8a983f530a3cf2efa8011be1e89cc610e3852d19f117d725
    flexibench check 59f8c892c7f8aaad77a198ee361fe5b36315993514ef61abd26ac558307745e6
    flexibench check-view bench.tex     # and serial, rtime, rwork, rsched, ridle, rsteals

These commands confirm three things:
- every object in a store has the digest it is stored under;
- each certificate holds for its store's log;
- each view shows exactly what its certificate certifies.

They don't check how the tables in the paper are laid out around the values, nor the
ratios that the paper's prose derives from them.
