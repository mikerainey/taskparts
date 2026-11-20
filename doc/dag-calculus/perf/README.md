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
| `dag` | 1144299 | `doc/dag-calculus/microtaskparts` | this paper's micro taskparts, in the rounds fib, ParlayLib and examples |
| `plain`, `sta`, `log`, `cnt`, `every` | 1144299 | `doc/dag-calculus/microtaskparts` | this paper's micro taskparts in the rounds cost and factors: with no more flags, `-DMTP_STATS`, `-DMTP_LOG`, `-DMTP_COUNT_FORKS -DMTP_COUNT_DAG`, and `-DMTP_MARK_EVERY_SWITCH` |

Commit 1144299 is on the local branch `perf-dag-regions`, made only for these
measurements: it is the branch `successor` at de00ccd with this directory's
`../microtaskparts` as it was when the rounds ran. Commit 5041c96 is on the native fork
join paper's measurement branch `perf-slots-direct`.

In the rounds fib and ParlayLib, the time of a run includes the start of the worker
threads, as it does with `nfj`, whose times it is compared with. In the other rounds, a
run that uses the scheduler is measured as a region, between two snapshots, and the
threads start before the first.

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
| fib | `.flexibench/` | `times` | `times` | `b209b6bbf01096ea92f9a7b39e67d70f49c48a293497f0658e3923e42ad3acdb` | `flexibench-dag-times` |
| ParlayLib | `parlay/.flexibench/` | `parlay` | `parlay` | `bd4f58a84dcb675d09862a18fe85b9f145be8e9189ca4e94f0536622ce3b5f28` | `flexibench-dag-parlay` |
| examples | `examples/.flexibench/` | `pipeline` | `pipeline` | `fbe23c09110664d948307be349dada38503a56d6394c7ea7bd45f1e1c9315d6b` | `flexibench-dag-pipeline` |
| examples | `examples/.flexibench/` | `stencil` | `stencil` | `49a8446a2999b8a53ef5b8422d0ddb6eefe423385181078cb534994cfa1c0938` | `flexibench-dag-stencil` |
| cost | `cost/.flexibench/` | `fib` | `costfib` | `83b48a591ebd252c076110c11ca3981d1311b56746acd7fe2b9e51d7269acf6b` | `flexibench-dag-cost` |
| cost | `cost/.flexibench/` | `loops` | `costloops` | `b6c82a869b546e4a8330d2519ccd150aa7d24f0a3b41eedfee1abbfb935574bd` | `flexibench-dag-cost` |
| cost | `cost/.flexibench/` | `each` | `costeach` | `466d374205d54f3c9f850129cc83870fd15bb3fd3a57d75665f69379ac066483` | `flexibench-dag-cost` |
| cost | `cost/.flexibench/` | `counts` | `cntswitches`, `cntforeign`, `cntretries` | `3bb5663c4a3f4c927ecc67935174d679a91edbb0a4f0f62d855555875f4f7497` | `flexibench-dag-cost` |
| factors | `factors/.flexibench/` | `ptimes` | `ptime` | `db542880270962c7154cc9fd42e8e7659db22c6a49300fecfda2ca64f4b88a5c` | `flexibench-dag-factors` |
| factors | `factors/.flexibench/` | `stimes` | `stime` | `054f3b3fd916025e29452e1d327c7e0b8c7b1f0a9efea5b6c97680ca629c74c7` | `flexibench-dag-factors` |
| factors | `factors/.flexibench/` | `ptotals` | `pwork`, `psched`, `pidle`, `psteals` | `0a42bba7fc5306fc020394e954f594f649a84cdac51d54cb1e721e8ee2666a8d` | `flexibench-dag-factors` |
| factors | `factors/.flexibench/` | `stotals` | `swork`, `ssched`, `sidle`, `ssteals` | `eabb92129ad7bce88fa00d25d838e529eb2035d431532c46b94ec2d729eabbe5` | `flexibench-dag-factors` |

On gamora:

| Round | Store | Analysis | Views | Certificate | Cited as |
|---|---|---|---|---|---|
| fib | `gamora/.flexibench/` | `times` | `timesgamora` | `4913f5c8ad4d3f4e2adae9ceeabb89c691aac447c6640de7cf8e5d0d515da940` | `flexibench-dag-times-gamora` |
| ParlayLib | `gamora/parlay/.flexibench/` | `parlay` | `parlaygamora` | `31fd60900e7da6d6ebbdffa502f02d965de33bcceaeea89ae17ad5a5ccc37558` | `flexibench-dag-parlay-gamora` |
| examples | `gamora/examples/.flexibench/` | `pipeline` | `pipelinegamora` | `5efed602dd4d53c2b2123c7755fa4131a52d7ced9034e882de620fa15e59756f` | `flexibench-dag-pipeline-gamora` |
| examples | `gamora/examples/.flexibench/` | `stencil` | `stencilgamora` | `43b04bf54bb1289ec5bbcfb12a75662184593fe5249e9b06ec2bc161fa391d1f` | `flexibench-dag-stencil-gamora` |
| cost | `gamora/cost/.flexibench/` | `fib` | `costfibgamora` | `80dbbc269829a9ef20b2af65fe92d3f7437f5a87e89b2d8d3cd583cb33213ae4` | `flexibench-dag-cost-gamora` |
| cost | `gamora/cost/.flexibench/` | `loops` | `costloopsgamora` | `3aaef23b80c7a31669158440718616b70f4b63a14e1d71f1cdcd2619d062c63f` | `flexibench-dag-cost-gamora` |
| cost | `gamora/cost/.flexibench/` | `each` | `costeachgamora` | `ebbece59439296031a9c9b87202bdc5467d78b06b8598bb122d4d75ac226fa42` | `flexibench-dag-cost-gamora` |
| cost | `gamora/cost/.flexibench/` | `counts` | `cntswitchesgamora`, `cntforeigngamora`, `cntretriesgamora` | `4b7004d1f59e39e028bbd149f87bb592ffdcd47e5bcae5bf353aa7caca1787c1` | `flexibench-dag-cost-gamora` |
| factors | `gamora/factors/.flexibench/` | `ptimes` | `ptimegamora` | `4df62a073a8b07208ce47db34bb1ccd53e2e5a32fdf0069321b8372c307813ee` | `flexibench-dag-factors-gamora` |
| factors | `gamora/factors/.flexibench/` | `stimes` | `stimegamora` | `c0db8d5c24ccffd312c45a77e2922f4f9b2fe9f28721144d4c478da0c252bacf` | `flexibench-dag-factors-gamora` |
| factors | `gamora/factors/.flexibench/` | `ptotals` | `pworkgamora`, `pschedgamora`, `pidlegamora`, `pstealsgamora` | `6ff31d75e5462540ab3c3bc880a33760fa1e0d5dc90c8ab1000702b3f1b10b2a` | `flexibench-dag-factors-gamora` |
| factors | `gamora/factors/.flexibench/` | `stotals` | `sworkgamora`, `sschedgamora`, `sidlegamora`, `sstealsgamora` | `a2b7990855f50d0642c6933f8e505d513f5197a24858a3fc8355ec23c90a362e` | `flexibench-dag-factors-gamora` |

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
  of at least 93% idle. The fib, ParlayLib, examples and cost rounds started that way.
  The factors round did not get them within seven minutes, and started with the machine
  between 91% and 95% idle.

The test suite of `../microtaskparts` (`test.sh`: 866 runs, in six builds with pauses at
random, the address and thread sanitizers, and all of the instrumentation) passed on both
machines at 1144299, but for one check of the script itself on gamora, since corrected:
it required every slice of a trace to show a duration above zero.

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

The rounds here replace those of this directory's history at commit 0974e21, which
measured `dag` at 45653bb and c9534d8. Since then, the worker threads outlive a
computation, a region is measured between two snapshots where a call to `launch` was,
and the sink tells the caller of `launch` to return as the last thing it does. Between
the two sets, the median difference of a time is 0.8% on either machine. On gamora, the
totals no longer add to the cost of a vertex: in the earlier rounds they added about
25 ns to an iteration of `finish` there.

One set of the rounds fib, ParlayLib and examples on the M5 was discarded: another load
ran on the machine during it, and the times of `nfj`, whose build had not changed, moved
by up to 20%.

Rounds before those are described in the READMEs of commits 0974e21 and 36c68bd.

Pilot choices, made in uncertified runs on the M5 before the first examples round: the
pipeline's hash takes eight bytes at a time and its compressor is of the LZ77 family, so
that compression dominates, as in dedup; the stencil's kernel is not inlined, since
inlined into the serial loop it compiled into code about 20% faster than elsewhere; and
the block size, 64, is the one at which the DAG version was fastest on 4 workers among
16, 64 and 256.
