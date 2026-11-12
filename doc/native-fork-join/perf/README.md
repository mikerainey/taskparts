# Measurements for the paper

The tables of `../main.tex` cite values that Flexibench generated from the measurements
recorded here. There are two rounds, each in a store of its own. Both were measured on
2026-11-12 on the Apple M5 described in the paper.

| Table | Store | Values | Citation | Certificate |
|---|---|---|---|---|
| `tbl:perf` | `.flexibench/` | `times.tex`, `times.html` | `times.bib` | `eaffe9bac32b911f6d8710c948bc0c9e0ec44dd4565a31c4378d52aa466c1a2d` |
| `tbl:parlay` | `parlay/.flexibench/` | `parlay/parlay.tex`, `parlay/parlay.html` | `parlay/parlay.bib` | `54048570e6b25ab8399cb72786c0bffe34a8a73c22324702a9228911d5c655fe` |

Both certificates were issued by Flexibench 2a10c0c with Lean 4.30.0. The two `.bib`
files are written by hand for now, so `check-view` doesn't check them.

After `\input{perf/times}`, `\fbval{times}{direct}{P = 4}` prints a value of the first
table, and after `\input{perf/parlay/parlay}`, `\fbval{parlay}{BFS}{direct 4}` one of
the second.

## Table `tbl:perf`: the cost of a fork

`fib 38` with three builds of `doc/native-fork-join/microtaskparts`, on 1, 4 and 8
workers, with five runs of each and the runs of the three builds interleaved. Each value
is the median of the five. The round also checked every build with `fib 27` and
`migrate`; every check passed.

| Row | Commit | What it is |
|---|---|---|
| `serial` | 67a207b | `fork2join` runs its two branches as two calls, with no scheduler |
| `locked` | 5761a33 | the paper's algorithm, with each deque protected by a lock |
| `direct` | bf99a97 | the paper's algorithm, with the lock-free deque: micro taskparts as it is |

Commits 67a207b and 5761a33 were made only for this table. They are on the branches
`perf-cost-serial` and `perf-guard-locked`. Commit bf99a97, on the branch `perf-parlay`,
has the same micro taskparts as this directory's parent. The `serial` build ignores the
number of workers, and the paper's table shows it for one worker only.

## Table `tbl:parlay`: programs that work between forks

Four example programs of ParlayLib (github.com/mikerainey/parlaylib, commit 80bf19d,
unmodified), each with three builds:

| Column | What it is |
|---|---|
| `serial` | ParlayLib with no scheduler (`PARLAY_SEQUENTIAL`) |
| `direct 1`, `direct 4` | ParlayLib on micro taskparts at bf99a97, through `doc/parlay-adapter`, on 1 and 4 workers |
| `homegrown 1`, `homegrown 4` | ParlayLib's own scheduler, on 1 and 4 workers |

The programs and their inputs: `mergesort` of 20 million integers, `samplesort` of 50
million, `quickhull` of 50 million points, and `BFS` of a generated graph with 2 million
vertices. Each program generates its input, then runs its algorithm 5 times (the sorts)
or 3 times in one process. Each program and build ran as five processes, interleaved.
A value is the median over the five processes of the median of a process's runs, its
first run included.

Every run's printed result was checked against the result the `serial` build prints.
The round also ran both schedulers on 2, 8 and 16 workers for that check alone. Every
check passed.

All builds use CMake's Release flags, `-O3 -DNDEBUG`, with `-falign-functions=64`.

## Earlier rounds

They are in this directory's history:
- up to commit 4e3070b, rounds that compared this algorithm with a formulation in which
  every fork passes through the scheduler loop;
- at commit 72b975e, the three builds of `tbl:perf` before `launch` was kept out of
  line. There, the compiler inlined `launch` into `fib`, which made every call of `fib`
  save eight more registers, and `direct` took 0.36 s on one worker;
- at commit 131f377, the same three builds before stolen-task stacks got a protected
  page. The times of `direct` were the same.

## Checking

With Flexibench 2a10c0c, from this directory:

    flexibench check-objects
    flexibench check eaffe9bac32b911f6d8710c948bc0c9e0ec44dd4565a31c4378d52aa466c1a2d
    flexibench check-view times.tex
    flexibench check-view times.html

and from `parlay/`:

    flexibench check-objects
    flexibench check 54048570e6b25ab8399cb72786c0bffe34a8a73c22324702a9228911d5c655fe
    flexibench check-view parlay.tex
    flexibench check-view parlay.html

These commands confirm three things:
- every object in a store has the digest it is stored under;
- the certificate holds for the store's log;
- each view shows exactly what the certificate certifies.

They don't check how the tables in the paper are laid out around the values.
