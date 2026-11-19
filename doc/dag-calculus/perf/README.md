# Measurements for the paper

The tables of `../main.tex` cite values in the files `*.tex` of this directory and its
subdirectories. Flexibench generated those files, the *views*, from the measurements
recorded in the stores `.flexibench/` beside them, and each view names the certificate
that certifies it.

There are five rounds, each run on two machines, all on 2026-11-19:

| Round | What it measures | Tables of the paper |
|---|---|---|
| fib | `fib 38` with the first paper's micro taskparts and with this one | `tbl:fork`, `tbl:fork16` |
| ParlayLib | four ParlayLib programs with the same two | `tbl:fork`, `tbl:fork16` |
| examples | the pipeline and the stencil | `tbl:examples`, `tbl:examples16` |
| cost | what the instrumentation costs, and what a vertex costs | `tbl:instrfib`, `tbl:vertexcost` |
| factors | where the time of the examples goes | `tbl:factors`, `tbl:factors16` |

The machines:

- an Apple M5 (4 performance and 6 efficiency cores, macOS; "the M5" in the paper), in
  this directory and `parlay/`, `examples/`, `cost/`, `factors/`;
- gamora, an AMD Ryzen 9 3950X (16 cores, 32 hardware threads, Linux 6.12; "the Ryzen"
  in the paper), in `gamora/` and its subdirectories of the same names. The views there
  have names that end in `gamora`, so that the paper can input them beside the M5's.

## Builds

Every build of a micro taskparts is made by `scripts/build-mtp-dir` of the measurement
client, from a commit of this repository and a micro taskparts directory, with the flags
`-std=c++17 -pthread -Wall -O2 -falign-functions=64`, plus the flags named below. The
ParlayLib programs (commit 80bf19d of the branch `microtaskparts` of
github.com/mikerainey/parlaylib) are built by `scripts/build-bench direct`, through
`doc/parlay-adapter`.

| Name | Commit | Directory | What it is |
|---|---|---|---|
| `serial` | 67a207b | `doc/native-fork-join/microtaskparts` | `fork2join` runs its two branches as two calls, with no scheduler |
| `nfj` | 5041c96 | `doc/native-fork-join/microtaskparts` | the native fork join paper's micro taskparts |
| `dag` | 45653bb | `doc/dag-calculus/microtaskparts` | this paper's micro taskparts, in the rounds fib, ParlayLib and examples |
| `plain`, `sta`, `log`, `cnt`, `every` | c9534d8 | `doc/dag-calculus/microtaskparts` | this paper's micro taskparts in the rounds cost and factors: with no more flags, `-DMTP_STATS`, `-DMTP_LOG`, `-DMTP_COUNT_FORKS -DMTP_COUNT_DAG`, and `-DMTP_MARK_EVERY_SWITCH` |

Commits 45653bb and c9534d8 are on the local branch `perf-dag-sink`, made only for these
measurements: each is the branch `successor` with this directory's `../microtaskparts` as
it was when the round ran. c9534d8 follows 45653bb, and differs from it only in the
instrumented builds (vertices are counted with `MTP_COUNT_DAG`, not with `MTP_STATS`) and
in a test: a build with no instrumentation compiles to the same code from both. Commit
5041c96 is on the native fork join paper's measurement branch `perf-slots-direct`.

## The rounds

- **fib.** `fib 38` with `serial`, `nfj` and `dag`, ten runs of each at each worker
  count, interleaved, and correctness checks of every build with `fib 27` and `migrate`.
- **ParlayLib.** Mergesort of 20 million integers, samplesort of 50 million, quickhull of
  50 million points, and BFS of a graph of 2 million vertices, five runs with `nfj` and
  `dag` at each worker count, interleaved, and once more at other worker counts for the
  check of the result only. ParlayLib's generators give other inputs on Linux than on
  macOS, so the expected results differ between the machines.
- **examples.** The pipeline (40000 chunks of 16384 bytes, with the default throttling
  limit, four times the number of workers), serially and in DAG calculus; and the
  stencil, with blocks of 64 by 64 cells, on a grid of 8192 by 8192 cells for 10 time
  steps (`large`) and on one of 1024 by 1024 for 400 (`small`), serially, with fork join
  (`planes`), and in DAG calculus (`dag`). Five runs of each configuration, interleaved.
  Every run checked its result against the serial version.
- **cost.** `fib 38` with `plain`, `sta`, `log` and `cnt`, on one worker, on all the
  cores, and on 4 workers (M5) or 32 (gamora); and the program `vertices`, with four
  million iterations, in its three modes: `forks`, `finish` and `each` on one worker with
  `plain`, `sta` and `every`; `forks` and `finish` with `plain` at every worker count;
  and all three with `cnt` at every worker count, for the counts. Ten runs of each,
  interleaved.
- **factors.** The plan of the examples round with the build `sta`, which prints the time
  that the workers spent working, scheduling and idle, and the count of steals.

Worker counts: 1, 2, 4, 6, 8 and 10 on the M5 (10 is all its cores), and 1, 2, 4, 6, 8,
10, 12, 14, 16 and 32 on gamora (16 is all its cores, 32 all its hardware threads).

Every check of every round passed: each round's analysis `failures` is certified, and
empty.

## Certificates

On the M5:

| Round | Store | Analysis | Views | Certificate | Cited as |
|---|---|---|---|---|---|
| fib | `.flexibench/` | `times` | `times` | `ea36be29d4488a5123da914ef359477a063dcff685377da851a02f05cbc2a1a1` | `flexibench-dag-times` |
| ParlayLib | `parlay/.flexibench/` | `parlay` | `parlay` | `f8e593bd0f2478b242d722b16f325b323e786828e5de4f88df2d1bb6d6fcbfa2` | `flexibench-dag-parlay` |
| examples | `examples/.flexibench/` | `pipeline` | `pipeline` | `b32dbfc433d28e45c737dc740af348ae84386454451acb80b105f09c0fd0ea79` | `flexibench-dag-pipeline` |
| examples | `examples/.flexibench/` | `stencil` | `stencil` | `948d2fed8df741e2e2f9660e55f8976436c11b6ef868df14fd151dad9c730b07` | `flexibench-dag-stencil` |
| cost | `cost/.flexibench/` | `fib` | `costfib` | `b11377f749f612993cd0485107aee181c30e07a7f008c5ee255aebcd41ae35de` | `flexibench-dag-cost` |
| cost | `cost/.flexibench/` | `loops` | `costloops` | `23b48d536e9ab162870d1ca035e2f7d9e7da9ef7a70115946f80be8df1ab3a84` | `flexibench-dag-cost` |
| cost | `cost/.flexibench/` | `each` | `costeach` | `08d1f67d22eb9f7d8a437e477c753c58321fe2f2fc1054f944e3432294096f41` | `flexibench-dag-cost` |
| cost | `cost/.flexibench/` | `counts` | `cntswitches`, `cntforeign`, `cntretries` | `e04b99c218080a40b944701a0e1263ca63a42feb35ae6682515583e9a728c595` | `flexibench-dag-cost` |
| factors | `factors/.flexibench/` | `ptimes` | `ptime` | `4ba9724ee020588fed8ea37d4ccee86aebf362d44afa6028fe4f69fbc8d9d093` | `flexibench-dag-factors` |
| factors | `factors/.flexibench/` | `stimes` | `stime` | `7997f8bcb281dfccd2688f6ea15041826f253366e018e8efc4602a2494ed4ce1` | `flexibench-dag-factors` |
| factors | `factors/.flexibench/` | `ptotals` | `pwork`, `psched`, `pidle`, `psteals` | `5d7e0ded3f81a493d852c70a4bff675ca6e4e079e6c0d8c8e5d5678ef9d889bd` | `flexibench-dag-factors` |
| factors | `factors/.flexibench/` | `stotals` | `swork`, `ssched`, `sidle`, `ssteals` | `7f405608eee2c79b57b9df779631785429b0afa9678e655b598d1b2f027be165` | `flexibench-dag-factors` |

On gamora:

| Round | Store | Analysis | Views | Certificate | Cited as |
|---|---|---|---|---|---|
| fib | `gamora/.flexibench/` | `times` | `timesgamora` | `0a3a94f4fbe6a76709468f31bfe19b5332b25c6dd452b5e6de90c39b953b98ac` | `flexibench-dag-times-gamora` |
| ParlayLib | `gamora/parlay/.flexibench/` | `parlay` | `parlaygamora` | `ac4125768321b4b3098ec118a8980c58cd863ce35f514b2b8d1b0400d057d84a` | `flexibench-dag-parlay-gamora` |
| examples | `gamora/examples/.flexibench/` | `pipeline` | `pipelinegamora` | `58b7674681dbb26ddc98db092ed8ba2a87e96094216a86be061f3673008a8889` | `flexibench-dag-pipeline-gamora` |
| examples | `gamora/examples/.flexibench/` | `stencil` | `stencilgamora` | `57d07d9a6b0cf79dad1b33062dc27da726945734e1c17adcd6ff6fb42f614327` | `flexibench-dag-stencil-gamora` |
| cost | `gamora/cost/.flexibench/` | `fib` | `costfibgamora` | `b8035b44316d2d3e1c60bc5e8f6e6e8ffdd4e542441d30bbcc5508e09f3f0393` | `flexibench-dag-cost-gamora` |
| cost | `gamora/cost/.flexibench/` | `loops` | `costloopsgamora` | `bb96ba53e5b9d437549876b7325ea02b4766619513aedb8ccf81e4eefc413582` | `flexibench-dag-cost-gamora` |
| cost | `gamora/cost/.flexibench/` | `each` | `costeachgamora` | `0c9e1004c30ba5a6ec227c0352e6ed2efcc3fe1fb0315ff6c781841e4a25ba30` | `flexibench-dag-cost-gamora` |
| cost | `gamora/cost/.flexibench/` | `counts` | `cntswitchesgamora`, `cntforeigngamora`, `cntretriesgamora` | `b1fe44cd2db595e2a4d807ec4969a0d85f1b84ada79701b9758dcc566ec5bc46` | `flexibench-dag-cost-gamora` |
| factors | `gamora/factors/.flexibench/` | `ptimes` | `ptimegamora` | `105a133d3ea775270cf21b9b20cdbd6dc3a26a37580966778f8439904f25fea8` | `flexibench-dag-factors-gamora` |
| factors | `gamora/factors/.flexibench/` | `stimes` | `stimegamora` | `cc8e5388f27cefb76aac91aa8baebbc7a63ab0cbff7acc1fd648d12d8fa5f1fb` | `flexibench-dag-factors-gamora` |
| factors | `gamora/factors/.flexibench/` | `ptotals` | `pworkgamora`, `pschedgamora`, `pidlegamora`, `pstealsgamora` | `aa3178bb67bed311ffe05e4b827e279017744dd6e95c0344717f2f3917f9c220` | `flexibench-dag-factors-gamora` |
| factors | `gamora/factors/.flexibench/` | `stotals` | `sworkgamora`, `sschedgamora`, `sidlegamora`, `sstealsgamora` | `b92a1b4be988642b1f16be8e3f58970c32ae2bc47f73050125ee2cdaaf8b742e` | `flexibench-dag-factors-gamora` |

The cost round has a view per count (`cntswitches`, `cntforeign`, `cntretries`): the
forks that switched to the scheduler loop, those whose pop found another task, and the
failed compare-and-swaps on an out-set. The factors round has a view per term: `ptime`
and `stime` (the time of the region, serial version included), and `pwork`, `psched`,
`pidle`, `psteals` and their `s` counterparts for the parallel versions; `p` is the
pipeline and `s` the stencil.

## The state of the machines

- **gamora** was in its benchmark mode during every round: the governor `performance` on
  all 32 CPUs, before and after each round, with frequency boost and SMT on. Each round
  started at 98% or 99% idle.
- **The M5** was in interactive use that day. A round waits for four consecutive samples
  of at least 93% idle. The fib, ParlayLib, cost and factors rounds started that way. The
  examples round did not get them, and was started at 90% idle at the user's request.
  Its times agree with those of the factors round, which ran the same programs an hour
  later, except for the pipeline on 10 workers (0.168 s here, 0.155 s there).

The test suite of `../microtaskparts` (`test.sh`: 830 runs, in six builds with pauses at
random, the address and thread sanitizers, and all of the instrumentation) passed on both
machines at c9534d8.

## Checking

With Flexibench 27e6e1e, from the directory of a store:

    flexibench check-objects
    flexibench check CERTIFICATE
    flexibench check-view VIEW.tex
    flexibench check-view VIEW.html

These commands confirm three things:
- every object in the store has the digest it is stored under;
- the certificate holds for the store's log;
- each view shows exactly what the certificate certifies.

They don't check how a table in the paper is laid out around the values, nor the
arithmetic of the paper's prose on them (a speedup, a ratio of two works).

## Earlier rounds

The rounds here replace those of this directory's history at commit 36c68bd, which
measured `dag` at 2a3f9db. Since then, a computation ends when a vertex that follows the
root runs, where a flag was set by the root; the instrumentation was added under build
flags; and a block of the stencil reads its fields before it counts a neighbor down,
where it read two of them after (the thread sanitizer reported it on gamora). The times
of the three rounds that both sets have agree within the variation between rounds.

Rounds before those are described in the README of commit 36c68bd.

Pilot choices, made in uncertified runs on the M5 before the first examples round: the
pipeline's hash takes eight bytes at a time and its compressor is of the LZ77 family, so
that compression dominates, as in dedup; the stencil's kernel is not inlined, since
inlined into the serial loop it compiled into code about 20% faster than elsewhere; and
the block size, 64, is the one at which the DAG version was fastest on 4 workers among
16, 64 and 256.
