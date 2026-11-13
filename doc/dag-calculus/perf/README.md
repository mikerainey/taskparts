# Measurements for the paper

Table `tbl:fork` of `../main.tex` cites the values in `times.tex` and
`parlay/parlay.tex`, and Table `tbl:examples` those in `examples/pipeline.tex` and
`examples/stencil.tex`. Flexibench generated those files from the measurements recorded
in the stores `.flexibench/`, `parlay/.flexibench/` and `examples/.flexibench/`.

The first two rounds ask one question: does hosting the vertices of DAG calculus slow
fork join down? They were measured on 2026-11-12 on an Apple M5 (4 performance and 6 efficiency
cores), with 1 and 4 workers.

## The fib round

`.flexibench/` holds `fib 38` with four builds, ten runs of each at each worker count,
with the runs of the builds interleaved, and correctness checks of every build with
`fib 27` and `migrate`, all of which passed. The view `times` (`times.tex`, `times.html`)
gives the median time of each build. It is certified by certificate
`34f3f65358a6a0a96b685af065b2d0a80c4051b69af693a62a8c977cafa1becd`, and cited as `flexibench-dag-times` (`times.bib`).

Every build is made by `scripts/build-mtp-dir` of the measurement client, from a commit of
this repository and a micro taskparts directory, with the flags
`-std=c++17 -pthread -Wall -O2 -falign-functions=64`.

| Row | Commit | Directory | What it is |
|---|---|---|---|
| `serial` | 67a207b | `doc/native-fork-join/microtaskparts` | `fork2join` runs its two branches as two calls, with no scheduler |
| `nfj` | d244863 | `doc/native-fork-join/microtaskparts` | the native fork join paper's micro taskparts |
| `nodtor` | bf0437b | `doc/native-fork-join/microtaskparts` | `nfj` with one change: a task's destructor does not free the task's stack, so stacks leak. For attribution only |
| `dag` | 3dc0e6d | `doc/dag-calculus/microtaskparts` | this paper's micro taskparts |

Commits bf0437b and 3dc0e6d are on the local branches `perf-dag-attrib` and `perf-dag`,
made only for these measurements.

## The ParlayLib round

`parlay/.flexibench/` holds four example programs of ParlayLib (commit 80bf19d of the
branch `microtaskparts` of github.com/mikerainey/parlaylib): mergesort of 20 million
integers, samplesort of 50 million, quickhull of 50 million points, and BFS of a graph
of 2 million vertices. Each program ran five times with each build at each worker
count, interleaved, and once more on 2, 8 and 16 workers for the check of its result
only. Every run printed the expected result. The view `parlay` (`parlay/parlay.tex`,
`parlay/parlay.html`) gives the median time of a run of the algorithm. It is certified by
certificate `671f6e3997d1de683b126e473a3731861df61d7baa9e310b9ff597820b6953b9`, and cited as `flexibench-dag-parlay` (`parlay/parlay.bib`).

Both builds are made by `scripts/build-bench direct`, through `doc/parlay-adapter`.

| Column | Commit | Directory |
|---|---|---|
| `nfj` | bf99a97 | `doc/native-fork-join/microtaskparts` (the same code as at d244863) |
| `dag` | 3dc0e6d | `doc/dag-calculus/microtaskparts` |

## The examples round

`examples/.flexibench/` holds the paper's two examples, measured on 2026-11-12 on the same
machine: five runs of each configuration, interleaved. Every run checked its own result
against the serial version, and all passed.

- The pipeline: 40000 chunks of 16384 bytes, with the default throttling limit (four
  times the number of workers), serially and in DAG calculus on 1 to 4 workers. The view
  `pipeline` (`examples/pipeline.tex`) is certified by certificate
  `d093475cd93cae9da85ef95928b1436cae8ac1d5122a18e635439b104f6e8d31`, and cited as `flexibench-dag-pipeline`.
- The stencil, with blocks of 64 by 64 cells, on a grid of 8192 by 8192 cells for 10 time
  steps (`large`) and on one of 1024 by 1024 for 400 (`small`): serially, with fork join
  (`planes`), and in DAG calculus (`dag`), on 1 to 4 workers. The view `stencil`
  (`examples/stencil.tex`) is certified by certificate
  `771078dcf07cb448ec9fa8688d5a23e6ccb488927664fc3086d9115bbe07dc30`, and cited as `flexibench-dag-stencil`.

Both citations are in `examples/examples.bib`. The build is `pipeline` and `seidel` of
`doc/dag-calculus/microtaskparts` at commit 6c34570 (branch `perf-dag`), made by
`PROGRAMS="pipeline seidel" scripts/build-mtp-dir`, with the flags of the fib round.

Two choices were made in uncertified pilot runs, before this round:
- The pipeline's hash and compressor. With a byte-at-a-time hash and run-length
  encoding, hashing, which is serial, was about four fifths of the work, and the
  pipeline could not speed up. The hash now takes eight bytes at a time and the
  compressor is of the LZ77 family, so that compression dominates, as in dedup.
- The stencil's kernel is not inlined. Inlined into the serial loop, it was compiled
  into code about 20% faster than elsewhere, which made the parallel versions look
  slower on one worker than their scheduling makes them. The block size, 64, is the one
  at which the DAG version was fastest on 4 workers among 16, 64 and 256.

## Checking

With Flexibench 2a10c0c, from this directory, then from `parlay/`, then from `examples/`:

    flexibench check-objects
    flexibench check CERTIFICATE
    flexibench check-view times.tex        (parlay.tex; pipeline.tex and stencil.tex)
    flexibench check-view times.html       (and likewise)

These commands confirm three things:
- every object in the store has the digest it is stored under;
- the certificate holds for the store's log;
- each view shows exactly what the certificate certifies.

They don't check how the table in the paper is laid out around the values.

## Earlier rounds

The rounds here are the second of each kind. The first, in this directory's history at
commit 341d263, measured an earlier `dag` (ce06cec), in which the scheduler loop counted
down the successors of a finished vertex; now the vertex does, from its own stack. The
medians of the two rounds of each kind differ by at most 8%, and rank the builds the
same way.

Before those, one attempt of the fib round was discarded. Its client had two more
analyses (the fastest run, and the ratio to `nfj`), which Flexibench did not certify, so
the round was run again without them. Its medians ranked the builds the same way too.

On four workers, where a run of `fib` lasts well under a tenth of a second, the three
rounds give `dag` 0.069 s, 0.078 s and 0.072 s. So there, differences of a few percent
between builds are within the variation between rounds.
