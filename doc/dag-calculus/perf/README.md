# Measurements for the paper

Tables `tbl:fork` and `tbl:examples` of `../main.tex` cite the values in `times.tex`,
`parlay/parlay.tex`, `examples/pipeline.tex` and `examples/stencil.tex`, and Tables
`tbl:fork16` and `tbl:examples16` those in the views under `gamora/`. Flexibench generated those files from the measurements recorded
in the stores `.flexibench/`, `parlay/.flexibench/` and `examples/.flexibench/`.

There are two sets of rounds, with the same programs, builds and inputs:

- on an Apple M5 (4 performance and 6 efficiency cores, macOS; "the M5" in the paper), measured on 2026-11-17, in
  this directory, `parlay/` and `examples/`;
- on gamora, an AMD Ryzen 9 3950X (16 cores, 32 hardware threads, Linux 6.12; "the Ryzen"
  in the paper), measured on
  2026-11-17, in `gamora/`, `gamora/parlay/` and `gamora/examples/`. See "The rounds on gamora"
  below.

The first two rounds of each set ask one question: does hosting the vertices of DAG
calculus slow fork join down?

## The fib round

`.flexibench/` holds `fib 38` with three builds, on 1, 2, 4, 6, 8 and 10 workers (10 is all the cores of the M5:
4 performance and 6 efficiency), ten runs of each at
each worker count, with the runs of the builds interleaved, and correctness checks of every
build with `fib 27` and `migrate`, all of which passed. The view `times` (`times.tex`,
`times.html`) gives the median time of each build. It is certified by certificate
`4abbce4a8236007c11688a8421c0ef9aa585881b6129bfe101a70853b1a03ca5`, and cited as
`flexibench-dag-times` (`times.bib`).

Every build is made by `scripts/build-mtp-dir` of the measurement client, from a commit of
this repository and a micro taskparts directory, with the flags
`-std=c++17 -pthread -Wall -O2 -falign-functions=64`.

| Row | Commit | Directory | What it is |
|---|---|---|---|
| `serial` | 67a207b | `doc/native-fork-join/microtaskparts` | `fork2join` runs its two branches as two calls, with no scheduler |
| `nfj` | 5041c96 | `doc/native-fork-join/microtaskparts` | the native fork join paper's micro taskparts |
| `dag` | 2a3f9db | `doc/dag-calculus/microtaskparts` | this paper's micro taskparts |

Commit 2a3f9db is on the local branch `perf-dag-pool`, made only for these measurements: it
is a74f3ee with this paper's `microtaskparts.hpp` as it is in this directory's commit.
Commit 5041c96 is on the native fork join paper's measurement branch `perf-slots-direct`:
its micro taskparts with the deque's slots left uninitialized, as ours leaves them.

## The ParlayLib round

`parlay/.flexibench/` holds four example programs of ParlayLib (commit 80bf19d of the
branch `microtaskparts` of github.com/mikerainey/parlaylib): mergesort of 20 million
integers, samplesort of 50 million, quickhull of 50 million points, and BFS of a graph
of 2 million vertices. Each program ran five times with each build at each worker
count, interleaved, and once more on 2, 8 and 16 workers for the check of its result
only. Every run printed the expected result. The view `parlay` (`parlay/parlay.tex`,
`parlay/parlay.html`) gives the median time of a run of the algorithm. It is certified by
certificate `25cb51dcd2fd5d0ff3ee4e264f35eecb18e3f3e2bd7d90f12a4fd1b443b0f290`, and cited
as `flexibench-dag-parlay` (`parlay/parlay.bib`).

Both builds are made by `scripts/build-bench direct`, through `doc/parlay-adapter`.

| Column | Commit | Directory |
|---|---|---|
| `nfj` | 5041c96 | `doc/native-fork-join/microtaskparts` |
| `dag` | 2a3f9db | `doc/dag-calculus/microtaskparts` |

## The examples round

`examples/.flexibench/` holds the paper's two examples: five runs of each configuration, interleaved. Every run checked its own result
against the serial version, and all passed.

- The pipeline: 40000 chunks of 16384 bytes, with the default throttling limit (four
  times the number of workers), serially and in DAG calculus on 1, 2, 4, 6, 8 and 10 workers. The view
  `pipeline` (`examples/pipeline.tex`) is certified by certificate
  `877f124ea23df9429d5d8b7495f791204d0dc5f7eb7899a1c816f116a5ed15a2`, and cited as `flexibench-dag-pipeline`.
- The stencil, with blocks of 64 by 64 cells, on a grid of 8192 by 8192 cells for 10 time
  steps (`large`) and on one of 1024 by 1024 for 400 (`small`): serially, with fork join
  (`planes`), and in DAG calculus (`dag`), on the same worker counts. The view `stencil`
  (`examples/stencil.tex`) is certified by certificate
  `d363c6f2730510b606db50d4a308eb969b3f2aaafd816fca5419dab595f5993d`, and cited as `flexibench-dag-stencil`.

Both citations are in `examples/examples.bib`. The build is `pipeline` and `seidel` of
`doc/dag-calculus/microtaskparts` at commit 2a3f9db (branch `perf-dag-pool`), made by
`PROGRAMS="pipeline seidel" scripts/build-mtp-dir`, with the flags of the fib round.

Two choices were made in uncertified pilot runs on the M5, before the first examples round:
- The pipeline's hash and compressor. With a byte-at-a-time hash and run-length
  encoding, hashing, which is serial, was about four fifths of the work, and the
  pipeline could not speed up. The hash now takes eight bytes at a time and the
  compressor is of the LZ77 family, so that compression dominates, as in dedup.
- The stencil's kernel is not inlined. Inlined into the serial loop, it was compiled
  into code about 20% faster than elsewhere, which made the parallel versions look
  slower on one worker than their scheduling makes them. The block size, 64, is the one
  at which the DAG version was fastest on 4 workers among 16, 64 and 256.

## The rounds on gamora

`gamora/` holds the same three rounds on gamora, with the same commits, flags and inputs,
built there with Clang 21.1.8. The differences:

- 1, 2, 4, 6, 8, 10, 12, 14, 16 and 32 workers for the timings (16 is all the cores, 32
  all the hardware threads), and 3, 24 and 64 (fib) or 3 and 64
  (ParlayLib) for the checks;
- ParlayLib's generators give other inputs on Linux, so the expected results are those
  the programs print there;
- the views have their own names, so that the paper can input them beside the M5's.

| Round | Store | View | Certificate | Cited as |
|---|---|---|---|---|
| fib | `gamora/.flexibench/` | `timesgamora` | `def44e05ae1d13ab9f12e127fec51129c1342116c038be483888768510492baa` | `flexibench-dag-times-gamora` |
| ParlayLib | `gamora/parlay/.flexibench/` | `parlaygamora` | `3c2f150d400ab2042d38476fe94d4548737e717cee85eecab5e8a7c2c379e58d` | `flexibench-dag-parlay-gamora` |
| examples | `gamora/examples/.flexibench/` | `pipelinegamora` | `1f0b637c5374148f1ceeeeee4543e5273e21521c16fa1d20553376ac113618c8` | `flexibench-dag-pipeline-gamora` |
| examples | `gamora/examples/.flexibench/` | `stencilgamora` | `ec0fe41c4b3cfe8944bedfc7fc0a25d34e5f123636c40f391de633f4cec6f763` | `flexibench-dag-stencil-gamora` |

Every check passed. During all three rounds the machine was in its benchmark mode: the
governor `performance` on all 32 CPUs, before and after each round, with frequency boost
and SMT on. Each round started at 98% or 99% idle. The test suite of `../microtaskparts`
(`test.sh`: 396 runs in the perturbed, address-sanitizer and thread-sanitizer builds) also
passed on gamora before the earlier rounds; the header has not changed since.

## Checking

With Flexibench 27e6e1e, from this directory, then from `parlay/`, then from `examples/`,
and likewise from `gamora/` and its two subdirectories:

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

The rounds here are the fourth of each kind on the M5 and the second on gamora, all
measured on 2026-11-17. They add worker counts (by twos, up to all the cores of each
machine), and compare against `nfj` at 5041c96.

- The rounds before these were never committed. They measured `nfj` at a74f3ee, on 1 and 4
  workers (M5; 1 to 4 for the examples) and on 1, 2, 4, 8, 16 and 32 (gamora). On gamora,
  `fib 38` took 0.042 s with `nfj` and 0.038 s and 0.033 s with `dag` on 16 and 32
  workers. The difference was a fixed cost at start-up: `nfj` initialized every slot of
  every worker's deque. At 5041c96 it does not, and the two builds take 0.038 s on 16
  workers and 0.034 s and 0.033 s on 32. Those rounds followed three changes to both
  micro taskparts: no destructor on a task, a pool of stacks shared by the workers, and
  a cheaper store in the deque's `push`.
- The second rounds, in this directory's history at commit 95fc62a, measured `dag` at
  3dc0e6d and 6c34570 against `nfj` at d244863, whose task had a destructor that looked for
  a stack to free at every return from `fork2join`. There `nfj` took 0.318 s and `dag`
  0.272 s for `fib 38` on one worker, and a build `nodtor` (bf0437b), which was `nfj` with
  that destructor emptied, took 0.282 s.
- The first rounds, in the history at commit 341d263, measured an earlier `dag` (ce06cec),
  in which the scheduler loop counted down the successors of a finished vertex; now the
  vertex does, from its own stack.
- Before those, one attempt of the fib round was discarded. Its client had two more
  analyses (the fastest run, and the ratio to `nfj`), which Flexibench did not certify.
- One ParlayLib round on the M5 was stopped and discarded on 2026-11-17: its client had
  failed to build, and the session had started the previous client, with 1 and 4 workers.
