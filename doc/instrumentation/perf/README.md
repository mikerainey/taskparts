# Measurements for the paper

The tables, plots and examples of `../main.tex` cite values that Flexibench generated from
the measurements recorded in six stores: three rounds on each of two machines. All were
measured on 2026-11-17, in the afternoon.

| | laptop | gamora |
|---|---|---|
| name in the paper | the M5 | the Ryzen |
| machine | Apple M5, 4 performance and 6 efficiency cores, macOS | AMD Ryzen 9 3950X, 16 cores, 32 hardware threads, Linux 6.12 |
| compiler | Apple clang 21.0.0 (clang-2100.4.1.1) | Clang 21.1.8 |
| Flexibench | 27e6e1e | 27e6e1e |
| stores | this directory, `bench/`, `curves/` | `gamora/`, `gamora/bench/`, `gamora/curves/` |
| view names | `times`, `bench`, `ctime`, ... | the same with `gamora` appended |
| workers | 1, 2, 4, 6, 8, 10 (10 is all its cores); curves on 1 to 10 | 1, 2, 4, 6, ..., 16 (16 is all its cores) and 32; curves on the same and 24 |

Lean is 4.30.0 on both. All the builds, on both machines, are from one commit of taskparts
on a local side branch: `4ff1190e` (`perf-instr6`). It is `successor` at a74f3ee0 plus this
paper's micro taskparts as it is in this directory's commit: the release-ordered store in
`push`, the shared pool of stacks, the array of packed counters in `fib.cpp` aligned to a
cache line, and deque slots that are not initialized.

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
| `times` | median time of each build at each worker count, to the millisecond (Table `tbl:cost`) | `f912b3f33a3dd1d74af14d042a5b99a805b9098f38b39ffdeec9ae946cfe8a69` |
| `work`, `sched`, `idle`, `steals` | medians of the totals that the build `stats` reported (the example of Appendix `sec:collect`) | `102e2099a196ab503282eaf1455143562c63933d107058ce941d5d25147d9b56` |
| `timesgamora` | the same on gamora (Table `tbl:costgamora`) | `e1761c62cfe1fb539aa3525109288c06d80ee556dddead315021e72a633864c0` |
| `workgamora`, `schedgamora`, `idlegamora`, `stealsgamora` | the totals on gamora | `0fe3d031cf172142e46a7ed7835f2651a5b6fe0b73f29e0f3d9ffcb1defb833f` |

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
| `bench` | time of a region, for each program, build and worker count (Table `tbl:bench`) | `2a220ca4c1f288ab81c0b46b66838f1ad995205291d2982233457020b548de79` |
| `rtime`, `rwork`, `rsched`, `ridle`, `rsteals` | for the build `stats`: the time of a region, and the work, scheduling and idle time and the steals it reported (Table `tbl:factors`) | `f1e2da70ad61b800ef4eb2b167aad9dd7f423fde71aa7c8de335e75b4b800a9f` |
| `serial` | time of a region with the serial build (Table `tbl:factors`) | `81eb414a4b38320c74690350b029451d5881b77278cff81a039505f9c4c7b3c4` |
| `benchgamora` | `bench` on gamora | `43956a5e5b34d92fca20d39b548909dec70c9d3f97820b818cbd272e6ddae744` |
| `rtimegamora`, `rworkgamora`, `rschedgamora`, `ridlegamora`, `rstealsgamora` | the totals on gamora (Table `tbl:factorsgamora`) | `25def6d21464d3ac0eca2cdd0e5fbfab5f18b061f140489c7596ce4dbceea282` |
| `serialgamora` | `serial` on gamora | `9effe11ed010efa7ef9764bbbb98c597141c81fd50fa6e43256a7a32ee17fbdf` |

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
| `ctime`, `cwork`, `csched`, `cidle` | for the build `stats`: the time of a region, and the work, scheduling and idle time it reported | `101575e7cb431538464ab9cf6ac646d9cd468760cf90d2ffc581472a5ef731f3` |
| `cserial` | time of a region with the serial build | `37dd51661d866689da62b6932a8eca9448737ce01a0ca8cf63d9df60dcf9fa1b` |
| `ctimegamora`, `cworkgamora`, `cschedgamora`, `cidlegamora` | the same on gamora | `4fee55ac8dc5f20877f73111b8f43e58d303489d93174463f4863805e17e8517` |
| `cserialgamora` | `cserial` on gamora | `8bf5d448c73912453a13e19d4dd3273c0da0b6116c490cb27170ea483cb8d803` |

`curves/curves.bib` is the citation `flexibench-curves`. The client is `mtpinstrcurves`
(`client/MtpInstrCurves.lean`). Check it from `curves/` as
the others, with `flexibench check-view ctime.tex` and so on.

## The clients

In the taskparts-bench repository, on its branch `gamora-instr` (local, not pushed):
`mtpinstr`, `mtpinstrbench` and `mtpinstrcurves` for the laptop, and `mtpinstrgamora`,
`mtpinstrbenchgamora` and `mtpinstrcurvesgamora` for gamora (`client/MtpInstr*.lean`),
with `scripts/build-mtp-instr` and `scripts/build-mtp-instr-bench`. All six were built
from that branch's flake, against Flexibench 27e6e1e; `runs/run-laptop-counts.sh` and
`runs/run-gamora-counts.sh` there are the two sessions, with their logs. `gamora/gamora.bib` is
the citation `flexibench-gamora`.

## Earlier rounds

Before these, the six rounds were run in the night of 2026-11-16 from commits 2d4c698b and
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

All six stores check with Flexibench 27e6e1e. In each of
the six directories that holds a `.flexibench/`:

    flexibench check-objects
    flexibench check <certificate>        # each certificate listed above for that store
    flexibench check-view <view>.tex      # each view, and its .html

These commands confirm three things:
- every object in a store has the digest it is stored under;
- each certificate holds for its store's log;
- each view shows exactly what its certificate certifies.

They don't check how the tables in the paper are laid out around the values, nor the
ratios that the paper's prose derives from them.
