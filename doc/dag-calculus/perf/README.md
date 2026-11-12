# Measurements for the paper

Table `tbl:fork` of `../main.tex` cites the values in `times.tex` and
`parlay/parlay.tex`. Flexibench generated those files from the measurements recorded in
the stores `.flexibench/` and `parlay/.flexibench/`.

Both rounds ask one question: does hosting the vertices of DAG calculus slow fork join
down? They were measured on 2026-11-12 on an Apple M5 (4 performance and 6 efficiency
cores), with 1 and 4 workers.

## The fib round

`.flexibench/` holds `fib 38` with four builds, ten runs of each at each worker count,
with the runs of the builds interleaved, and correctness checks of every build with
`fib 27` and `migrate`, all of which passed. The view `times` (`times.tex`, `times.html`)
gives the median time of each build. It is certified by certificate
`614598c1ca5c3e1c5c9180c0d2ff1eac361fd068631b1e0ba386e5edca7131dc`, and cited as `flexibench-dag-times` (`times.bib`).

Every build is made by `scripts/build-mtp-dir` of the measurement client, from a commit of
this repository and a micro taskparts directory, with the flags
`-std=c++17 -pthread -Wall -O2 -falign-functions=64`.

| Row | Commit | Directory | What it is |
|---|---|---|---|
| `serial` | 67a207b | `doc/native-fork-join/microtaskparts` | `fork2join` runs its two branches as two calls, with no scheduler |
| `nfj` | d244863 | `doc/native-fork-join/microtaskparts` | the native fork join paper's micro taskparts |
| `nodtor` | bf0437b | `doc/native-fork-join/microtaskparts` | `nfj` with one change: a task's destructor does not free the task's stack, so stacks leak. For attribution only |
| `dag` | ce06cec | `doc/dag-calculus/microtaskparts` | this paper's micro taskparts |

Commits bf0437b and ce06cec are on the local branches `perf-dag-attrib` and `perf-dag`,
made only for these measurements.

## The ParlayLib round

`parlay/.flexibench/` holds four example programs of ParlayLib (commit 80bf19d of the
branch `microtaskparts` of github.com/mikerainey/parlaylib): mergesort of 20 million
integers, samplesort of 50 million, quickhull of 50 million points, and BFS of a graph
of 2 million vertices. Each program ran five times with each build at each worker
count, interleaved, and once more on 2, 8 and 16 workers for the check of its result
only. Every run printed the expected result. The view `parlay` (`parlay/parlay.tex`,
`parlay/parlay.html`) gives the median time of a run of the algorithm. It is certified by
certificate `d98e3a2c6e4fb3d44ebbde81dfdbfda0d853b896613a8d2d36fd91ba4a00ed14`, and cited as `flexibench-dag-parlay` (`parlay/parlay.bib`).

Both builds are made by `scripts/build-bench direct`, through `doc/parlay-adapter`.

| Column | Commit | Directory |
|---|---|---|
| `nfj` | bf99a97 | `doc/native-fork-join/microtaskparts` (the same code as at d244863) |
| `dag` | ce06cec | `doc/dag-calculus/microtaskparts` |

## Checking

With Flexibench 2a10c0c, from this directory and then from `parlay/`:

    flexibench check-objects
    flexibench check CERTIFICATE
    flexibench check-view times.tex        (parlay.tex in parlay/)
    flexibench check-view times.html       (parlay.html in parlay/)

These commands confirm three things:
- every object in the store has the digest it is stored under;
- the certificate holds for the store's log;
- each view shows exactly what the certificate certifies.

They don't check how the table in the paper is laid out around the values.

## Not in these stores

An earlier attempt of the fib round is not kept here. Its client had two more analyses
(the fastest run and the ratio to `nfj`), which Flexibench did not certify, so the round
was run again without them. Its medians ranked the builds the same way, and were lower
than this round's by 2% to 5% on one worker and by up to 11% on four (`dag`: 0.069 s
against 0.078 s). So on four workers, where a run lasts well under a tenth of a second,
differences of a few percent between builds are within the variation between rounds.
