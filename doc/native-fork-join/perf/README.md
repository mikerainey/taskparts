# Measurements for the paper

The tables of `../main.tex` cite values that Flexibench generated from the measurements
recorded here. There are four rounds, each in a store of its own. Two were measured on
2026-11-12 on the Apple M5 described in the paper, and two on 2026-11-16 on a second
machine (below).

| Table | Store | Values | Citation | Certificate |
|---|---|---|---|---|
| `tbl:perf` | `.flexibench/` | `times.tex`, `times.html` | `times.bib` | `5314e2487158940880f53da703f58c831be8a7a44aca7b845eefe61e3170804a` |
| `tbl:parlay` | `parlay/.flexibench/` | `parlay/parlay.tex`, `parlay/parlay.html` | `parlay/parlay.bib` | `a111592ab4c17839f4c63f0227800b38187fd3712661ff4511e537449378256c` |

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
| `locked` | b48599d | the paper's algorithm, with each deque protected by a lock |
| `direct` | 958b792 | the paper's algorithm, with the lock-free deque: micro taskparts as it is |

Commits 67a207b and b48599d were made only for this table. They are on the branches
`perf-cost-serial` and `perf-stack-locked`. Commit 958b792, on the branch
`perf-stack-direct`, has the micro taskparts of this directory's parent, except for one
later change: `push` now stores the deque's bottom index with release ordering instead
of sequentially consistent ordering. On ARM64, the machine these tables were measured
on, the compiler emits the same instructions for `fib` and `migrate` either way, so the
tables were not measured again. On x86-64 the change removes a locked instruction from
every fork. The `serial` build ignores the
number of workers, and the paper's table shows it for one worker only.

## Table `tbl:parlay`: programs that work between forks

Four example programs of ParlayLib (github.com/mikerainey/parlaylib, commit 80bf19d,
unmodified), each with three builds:

| Column | What it is |
|---|---|
| `serial` | ParlayLib with no scheduler (`PARLAY_SEQUENTIAL`) |
| `direct 1`, `direct 4` | ParlayLib on micro taskparts at 958b792, through `doc/parlay-adapter`, on 1 and 4 workers |
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

## Table `tbl:second`: a second machine

Both experiments again, on gamora: an AMD Ryzen 9 3950X with 16 cores and 32 hardware
threads, with Linux 6.12 (NixOS) and Clang 21. Measured on 2026-11-16, in one session,
with the frequency governor set to `performance`, frequency boost on, and both hardware
threads of each core enabled. The paper's table shows 1 and 16 workers; the views also
hold 2, 4, 8 and 32.

| Store | Values | Citation | Certificate |
|---|---|---|---|
| `gamora/.flexibench/` | `gamora/timesgamora.tex`, `.html` | `gamora/timesgamora.bib` | `afbd206a3380d65cb0a8c1bcc2d9e8c239f06f5fed97e0eb089500d807027a8c` |
| `gamora/parlay/.flexibench/` | `gamora/parlay/parlaygamora.tex`, `.html` | `gamora/parlay/parlaygamora.bib` | `1ccef155b08d1ce3a2e5bc6d75670a8435c98c5329eae0fa9b18b0efcbdb30f0` |

Both certificates were issued on gamora by Flexibench 27e6e1e, the first revision that
builds on Linux, with Lean 4.30.0. `\fbval{timesgamora}{direct}{P = 16}` and
`\fbval{parlaygamora}{BFS}{direct 16}` print values of the two views.

The builds are those of the first two tables, compiled on gamora, with one difference:
`direct` is micro taskparts at commit 1716d8e, on the branch `perf-push-direct`, which is
958b792 plus the release-ordered store in `push` described above, and so the micro
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
- at commit d244863, both tables before the destructor of a task was removed. There,
  every return from `fork2join` ran a destructor that freed the stolen task's stack, if
  any, and `direct` took 0.31 s on one worker. Now the stolen path alone frees it.

## Checking

With Flexibench 2a10c0c, from this directory:

    flexibench check-objects
    flexibench check 5314e2487158940880f53da703f58c831be8a7a44aca7b845eefe61e3170804a
    flexibench check-view times.tex
    flexibench check-view times.html

and from `parlay/`:

    flexibench check-objects
    flexibench check a111592ab4c17839f4c63f0227800b38187fd3712661ff4511e537449378256c
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
