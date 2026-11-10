# Measurements for the paper

Table `tbl:perf` of `../main.tex` cites the values in `times.tex`. Flexibench generated
that file from the measurements recorded in the store `.flexibench/`.

- `.flexibench/`: the Flexibench store of one round, measured on 2026-11-09 on the
  Apple M5 described in the paper. It holds:
  - `fib 38` with seven builds of micro taskparts, on 1, 4 and 8 workers, with five runs of
    each and the runs of the seven builds interleaved;
  - correctness checks of every build, with `fib 27` and `migrate`, all of which passed.
- `times.tex`: the view `times`, the median time of each build at each worker count, as
  LaTeX macros. After `\input{perf/times}`, `\fbval{times}{swap}{P = 4}` prints a value
  and `\fbcertificate{times}` prints the certificate's digest.
- `times.html`: the same view, as a web page.
- `times.bib`: the paper's citation of the view, `flexibench-times`, which gives the
  certificate's digest. It is written by hand for now, so `check-view` doesn't check it.

The view is certified by certificate
`a5cf1c2f58147214740c9af4ea1a575e5a9f08cb18efc5bd0dc1ad28cd5b684d`, issued by Flexibench
2a10c0c with Lean 4.30.0.

## The builds

Each row of the table is a build of `doc/native-fork-join/microtaskparts`.

| Row | Commit | Change |
|---|---|---|
| `lock` | 878a509 | the deque protected by a lock |
| `lockfree` | 9764023 | a lock-free deque; each worker chooses victims with its own `std::minstd_rand` |
| `hash` | a407ae3 | victims chosen by hashing, as in TaskPaRTS |
| `lines` | a22aa14 | the in-counter, the out-set and the context of a vertex each start a cache line |
| `lookups` | 5c7831a | the worker looked up only where it may have changed |
| `swap` | 7fb2c44 | a context switch in one call, resuming with a jump |
| `direct` | 23adce6 | no round trips through the scheduler loop: the algorithm of the paper's Section 2 |

The first six builds implement the earlier formulation of the paper's appendix "Fork
join through the scheduler". Each is the one before it with one change, and `direct`
replaces that formulation with the paper's algorithm.

Commits 9764023, a407ae3, a22aa14 and 23adce6 were made only for this table. They are on
the branches `perf-step-minstd`, `perf-steps` and `perf-direct`.

## Checking

From this directory, with Flexibench 2a10c0c:

    flexibench check-objects
    flexibench check a5cf1c2f58147214740c9af4ea1a575e5a9f08cb18efc5bd0dc1ad28cd5b684d
    flexibench check-view times.tex
    flexibench check-view times.html

These commands confirm three things:
- every object in the store has the digest it is stored under;
- the certificate holds for the store's log;
- each view shows exactly what the certificate certifies.

They don't check how the table in the paper is laid out around the values.
