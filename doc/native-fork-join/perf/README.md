# Measurements for the paper

Table `tbl:perf` of `../main.tex` cites the values in `times.tex`. Flexibench generated
that file from the measurements recorded in the store `.flexibench/`.

- `.flexibench/`: the Flexibench store of one round, measured on 2026-11-10 on the
  Apple M5 described in the paper. It holds:
  - `fib 38` with three builds of micro taskparts, on 1, 4 and 8 workers, with five runs of
    each and the runs of the three builds interleaved;
  - correctness checks of every build, with `fib 27` and `migrate`, all of which passed.
- `times.tex`: the view `times`, the median time of each build at each worker count, as
  LaTeX macros. After `\input{perf/times}`, `\fbval{times}{direct}{P = 4}` prints a value
  and `\fbcertificate{times}` prints the certificate's digest.
- `times.html`: the same view, as a web page.
- `times.bib`: the paper's citation of the view, `flexibench-times`, which gives the
  certificate's digest. It is written by hand for now, so `check-view` doesn't check it.

The view is certified by certificate
`a9215fd2f398e3e352da83902774aca1f626a0654ab83cbff6fbb627c0b34cf2`, issued by Flexibench
2a10c0c with Lean 4.30.0.

## The builds

Each row of the table is a build of `doc/native-fork-join/microtaskparts`.

| Row | Commit | What it is |
|---|---|---|
| `serial` | 67a207b | `fork2join` runs its two branches as two calls, with no scheduler |
| `locked` | 48b1fa8 | the paper's algorithm, with each deque protected by a lock |
| `direct` | 4cdd557 | the paper's algorithm, with the lock-free deque: micro taskparts as it is |

Commits 67a207b and 48b1fa8 were made only for this table. They are on the branches
`perf-cost-serial` and `perf-launch-locked`. Commit 4cdd557, on the branch
`perf-launch-direct`, has the same micro taskparts as this directory's parent. The
`serial` build ignores the number of workers, and the paper's table shows it for one
worker only.

Earlier rounds are in this directory's history:
- up to commit 4e3070b, rounds that compared this algorithm with a formulation in which
  every fork passes through the scheduler loop;
- at commit 72b975e, the same three builds before `launch` was kept out of line. There,
  the compiler inlined `launch` into `fib`, which made every call of `fib` save eight
  more registers, and `direct` took 0.36 s on one worker.

## Checking

From this directory, with Flexibench 2a10c0c:

    flexibench check-objects
    flexibench check a9215fd2f398e3e352da83902774aca1f626a0654ab83cbff6fbb627c0b34cf2
    flexibench check-view times.tex
    flexibench check-view times.html

These commands confirm three things:
- every object in the store has the digest it is stored under;
- the certificate holds for the store's log;
- each view shows exactly what the certificate certifies.

They don't check how the table in the paper is laid out around the values.
