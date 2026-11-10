# Measurements for the paper

Table `tbl:perf` of `../main.tex` cites the values in `times.tex`. Flexibench generated
that file from the measurements recorded in the store `.flexibench/`.

- `.flexibench/`: the Flexibench store of one round, measured on 2026-11-09 on the
  Apple M5 described in the paper. It holds:
  - `fib 38` with six builds of micro taskparts, on 1, 4 and 8 workers, with five runs of
    each and the runs of the six builds interleaved;
  - correctness checks of every build, with `fib 27` and `migrate`, all of which passed.
- `times.tex`: the view `times`, the median time of each build at each worker count, as
  LaTeX macros. After `\input{perf/times}`, `\fbval{times}{swap}{P = 4}` prints a value
  and `\fbcertificate{times}` prints the certificate's digest.
- `times.html`: the same view, as a web page.

The view is certified by certificate
`df59c9d1b458e4c9de65cf06ff47b54d6ec0ac9711ff30cb9f560694a704cfdd`, issued by Flexibench
2a10c0c with Lean 4.30.0.

## The builds

Each row of the table is a build of `doc/native-fork-join/microtaskparts`, and each build
is the one before it with one change.

| Row | Commit | Change |
|---|---|---|
| `lock` | 878a509 | the deque protected by a lock |
| `lockfree` | 9764023 | a lock-free deque; each worker chooses victims with its own `std::minstd_rand` |
| `hash` | a407ae3 | victims chosen by hashing, as in TaskPaRTS |
| `lines` | a22aa14 | the in-counter, the out-set and the context of a vertex each start a cache line |
| `lookups` | 5c7831a | the worker looked up only where it may have changed |
| `swap` | 7fb2c44 | a context switch in one call, resuming with a jump |

Commits 9764023, a407ae3 and a22aa14 were made only for this table. They are on the
branches `perf-step-minstd` and `perf-steps`.

## Checking

From this directory, with Flexibench 2a10c0c:

    flexibench check-objects
    flexibench check df59c9d1b458e4c9de65cf06ff47b54d6ec0ac9711ff30cb9f560694a704cfdd
    flexibench check-view times.tex
    flexibench check-view times.html

These commands confirm three things:
- every object in the store has the digest it is stored under;
- the certificate holds for the store's log;
- each view shows exactly what the certificate certifies.

They don't check how the table in the paper is laid out around the values.
