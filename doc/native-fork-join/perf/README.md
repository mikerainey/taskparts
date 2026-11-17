# Measurements for the paper

The tables of `../main.tex` cite values that Flexibench generated from the measurements
recorded here. There are four rounds, each in a store of its own. Two were measured on
2026-11-17 on the Apple M5 described in the paper, and two on the same day on a second
machine (below).

The paper names the machines by their processors (its Table `tbl:machines`): the M5 is
the laptop, and the Ryzen is the machine called gamora here and in the file names.

| Table | Store | Values | Citation | Certificate |
|---|---|---|---|---|
| `tbl:perf` | `.flexibench/` | `times.tex`, `times.html` | `times.bib` | `8ea7d0b4ce378256218159ee8fef96443320a10736ded86f0057fe42bee5d802` |
| `tbl:parlay` | `parlay/.flexibench/` | `parlay/parlay.tex`, `parlay/parlay.html` | `parlay/parlay.bib` | `192e62ff1d0f8bfca7ce49d994a9523530810063f670594a8c0bf657a69d49be` |

Both certificates were issued by Flexibench 2a10c0c with Lean 4.30.0. The two `.bib`
files are written by hand for now, so `check-view` doesn't check them.

After `\input{perf/times}`, `\fbval{times}{direct}{P = 4}` prints a value of the first
table, and after `\input{perf/parlay/parlay}`, `\fbval{parlay}{BFS}{direct 4}` one of
the second.

## Table `tbl:perf`: the cost of a fork

`fib 38` with three builds of `doc/native-fork-join/microtaskparts`, on 1, 4, 8 and 10
workers, with five runs of each and the runs of the three builds interleaved. Each value
is the median of the five. The round also checked every build with `fib 27` and
`migrate`; every check passed.

| Row | Commit | What it is |
|---|---|---|
| `serial` | 67a207b | `fork2join` runs its two branches as two calls, with no scheduler |
| `locked` | f0767d3 | the paper's algorithm, with each deque protected by a lock |
| `direct` | 5041c96 | the paper's algorithm, with the lock-free deque: micro taskparts as it is |

Commits 67a207b and f0767d3 were made only for this table. They are on the branches
`perf-cost-serial` and `perf-sharedpool-locked`. Commit 5041c96, on the branch
`perf-slots-direct`, has the same micro taskparts as this directory's parent.
The `serial` build ignores the
number of workers, and the paper's table shows it for one worker only.

## Table `tbl:parlay`: programs that work between forks

Four example programs of ParlayLib (github.com/mikerainey/parlaylib, commit 80bf19d,
unmodified), each with three builds:

| Column | What it is |
|---|---|
| `serial` | ParlayLib with no scheduler (`PARLAY_SEQUENTIAL`) |
| `direct 1`, `direct 4`, `direct 10` | ParlayLib on micro taskparts at 5041c96, through `doc/parlay-adapter`, on 1, 4 and 10 workers |
| `homegrown 1`, `homegrown 4`, `homegrown 10` | ParlayLib's own scheduler, on 1, 4 and 10 workers |

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

## Table `tbl:second`: a second machine

Both experiments again, on gamora: an AMD Ryzen 9 3950X with 16 cores and 32 hardware
threads, with Linux 6.12 (NixOS) and Clang 21. Measured on 2026-11-17, in one session,
with the frequency governor set to `performance`, frequency boost on, and both hardware
threads of each core enabled. The paper's table shows 1 and 16 workers; the views also
hold 2, 4, 8 and 32.

| Store | Values | Citation | Certificate |
|---|---|---|---|
| `gamora/.flexibench/` | `gamora/timesgamora.tex`, `.html` | `gamora/timesgamora.bib` | `6b1be25ac0ca1597e2ecf70aa7f79b5b1af3e6d3666955ffbb6a06477c61373a` |
| `gamora/parlay/.flexibench/` | `gamora/parlay/parlaygamora.tex`, `.html` | `gamora/parlay/parlaygamora.bib` | `848d515a9d20b7f660124bc1ec9083fac7a18dbcd0ec0492ade327d1a45e5319` |

Both certificates were issued on gamora by Flexibench 27e6e1e, the first revision that
builds on Linux, with Lean 4.30.0. `\fbval{timesgamora}{direct}{P = 16}` and
`\fbval{parlaygamora}{BFS}{direct 16}` print values of the two views.

The builds are those of the first two tables, compiled on gamora, with one difference:
`locked` and `direct` are micro taskparts at commits f0767d3 and 5041c96, on the branches
`perf-sharedpool-locked` and `perf-slots-direct`. Commit 5041c96 has the micro
taskparts of this directory's parent. The programs and their input sizes are the same,
but ParlayLib's generators give other inputs on Linux than on macOS, so the results the
round checked against are the ones the `serial` build prints on Linux. The round ran
each program and build as five processes, and checked both schedulers on 3 and 64
workers as well. Every check passed.

The clients are `mtpperfgamora` and `mtpparlaygamora`, on the branch `gamora` of the
measurement repository.

## Earlier rounds

They are in this directory's history:
- up to commit 4e3070b, rounds that compared this algorithm with a formulation in which
  every fork passes through the scheduler loop;
- at commit 72b975e, the three builds of `tbl:perf` before `launch` was kept out of
  line. There, the compiler inlined `launch` into `fib`, which made every call of `fib`
  save eight more registers, and `direct` took 0.36 s on one worker;
- at commit 131f377, the same three builds before stolen-task stacks got a protected
  page. The times of `direct` were the same;
- at commit 524b1aa, the laptop's two tables before stacks were pooled and before `push`
  stored the deque's bottom index with release ordering (builds b48599d and 958b792). The
  times were the same to within 5%;
- at commit 524b1aa, the second machine's rounds before stacks were pooled (certificates
  `afbd206a…` and `1ccef155…`). There, every steal mapped a stack and the join of a
  stolen task unmapped it, and on 16 workers `quickhull` took 21% longer and `BFS` 74%
  longer with micro taskparts than with ParlayLib's scheduler;
- at commit 40eaecb, all four rounds before the slots of a deque were left
  uninitialized (build 16d4e28 in place of 5041c96). There, starting the workers wrote
  640 KB for each of them, which took about 10 ms for 32 workers on the Ryzen, and
  `fib 38` took 0.04 s there on 32 workers instead of 0.03 s. The other times were the
  same to within 4%. The laptop's rounds then had no runs on 10 workers;
- at commit d244863, both tables before the destructor of a task was removed. There,
  every return from `fork2join` ran a destructor that freed the stolen task's stack, if
  any, and `direct` took 0.31 s on one worker. Now the stolen path alone frees it.

## Checking

With Flexibench 2a10c0c, from this directory:

    flexibench check-objects
    flexibench check 8ea7d0b4ce378256218159ee8fef96443320a10736ded86f0057fe42bee5d802
    flexibench check-view times.tex
    flexibench check-view times.html

and from `parlay/`:

    flexibench check-objects
    flexibench check 192e62ff1d0f8bfca7ce49d994a9523530810063f670594a8c0bf657a69d49be
    flexibench check-view parlay.tex
    flexibench check-view parlay.html

and, with Flexibench 27e6e1e, from `gamora/` and from `gamora/parlay/`:

    flexibench check-objects
    flexibench check-view timesgamora.tex      (parlaygamora.tex in gamora/parlay/)

These commands confirm three things:
- every object in a store has the digest it is stored under;
- the certificate holds for the store's log;
- each view shows exactly what the certificate certifies.

They don't check how the tables in the paper are laid out around the values.

## Rounds that were set aside

A first round of the laptop's two tables with the pooled stacks, earlier on 2026-11-17,
ran while the laptop was in interactive use: the round script checked for an idle machine
only once, at its start. `BFS` on 4 workers took 0.085 s with micro taskparts there,
against 0.073 s with ParlayLib's scheduler, and by hand the difference did not reproduce.
The rounds recorded here started after four consecutive samples at 93% idle or better.
The first round is not in this directory.

On the same day, with build 5041c96, the laptop's two rounds first ran without 10
workers. In the ParlayLib round, `samplesort` on 4 workers took 0.266 s with micro
taskparts against 0.242 s with ParlayLib's scheduler; the laptop was in use during the
round, and by hand both took 0.24 to 0.25 s. A rerun was stopped a third of the way
through, when 10 workers were added to the plan. The rounds recorded here are the ones
with 10 workers, which came next. The others are not in this directory.
