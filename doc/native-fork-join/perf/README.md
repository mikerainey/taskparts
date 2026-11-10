# Measurements for the paper

Table `tbl:perf` of `../main.tex` cites the values in `times.tex`. Flexibench generated
that file from the measurements recorded in the store `.flexibench/`.

- `.flexibench/`: the Flexibench store of one round, measured on 2026-11-09 on the
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
`e402d828ae9113cc9c95f4dfc9b93d16ad57edadaf38b51912301bab211a7d81`, issued by Flexibench
2a10c0c with Lean 4.30.0.

## The builds

Each row of the table is a build of `doc/native-fork-join/microtaskparts`.

| Row | Commit | What it is |
|---|---|---|
| `serial` | 67a207b | `fork2join` runs its two branches as two calls, with no scheduler |
| `locked` | 1c29ce5 | the paper's algorithm, with each deque protected by a lock |
| `direct` | a82c042 | the paper's algorithm, with the lock-free deque: micro taskparts as it is |

Commits 67a207b and 1c29ce5 were made only for this table. They are on the branches
`perf-cost-serial` and `perf-cost-locked`. The `serial` build ignores the number of
workers, and the paper's table shows it for one worker only.

Earlier rounds, which compared this algorithm with a formulation in which every fork
passes through the scheduler loop, are in this directory's history, up to commit 4e3070b.

## Checking

From this directory, with Flexibench 2a10c0c:

    flexibench check-objects
    flexibench check e402d828ae9113cc9c95f4dfc9b93d16ad57edadaf38b51912301bab211a7d81
    flexibench check-view times.tex
    flexibench check-view times.html

These commands confirm three things:
- every object in the store has the digest it is stored under;
- the certificate holds for the store's log;
- each view shows exactly what the certificate certifies.

They don't check how the table in the paper is laid out around the values.
