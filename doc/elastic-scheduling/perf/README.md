# Measurements for the paper

The tables of `../main.tex` cite values that Flexibench generated from the measurements
recorded here. There are seven rounds, each in a store of its own: two on the Apple M5,
two on the machine the papers call the Ryzen, which is called gamora here and in the
file names (an AMD Ryzen 9 3950X; see `doc/native-fork-join/perf/README.md`), and three on
the machine the papers call the Xeon, which is called icelake here and in the file names
(two Intel Xeon Platinum 8380, 80 cores, 160 hardware threads, 125 GB, Ubuntu 24.04). The
all ran on 2026-11-22, and the M5's and the Ryzen's are second runs of rounds first
made on 2026-11-19 with other builds (below).

| Round | Store | Views | Citation | Certificate |
|---|---|---|---|---|
| `fib`, M5 | `fib/.flexibench/` | `fib/efib.tex` | `fib/efib.bib` | `7b4368d026a1bf0f418f300a76c6744f58e16781db86a6671a269483552da5e2` |
| programs, M5 | `parlay/.flexibench/` | `parlay/etime.tex`, `euser.tex`, `esys.tex` | `parlay/eparlay.bib` | `edd7e3813b765c2bd5c7308ad4d66fc28d6c472a5b3f3c91c0e1e3c10746f721` |
| `fib`, Ryzen | `gamora/fib/.flexibench/` | `gamora/fib/efibgamora.tex` | `gamora/fib/efibgamora.bib` | `5dffef56c3babe9afa63ed8db8d658cddfd8ff7e5932fca3e32c8b1b77eba3ab` |
| programs, Ryzen | `gamora/parlay/.flexibench/` | `gamora/parlay/etimegamora.tex`, `eusergamora.tex`, `esysgamora.tex` | `gamora/parlay/eparlaygamora.bib` | `c77ab681e2ad5d373e498c174b7dd9686ab0c44914f56f7566121b5d1c703abd` |
| `fib`, Xeon | `icelake/fib/.flexibench/` | `icelake/fib/efibicelake.tex` | `icelake/fib/efibicelake.bib` | `11cb490a43ee3e8c0a2b85e4d40137de8608a42f9a031918d23945011545245a` |
| programs, Xeon | `icelake/parlay/.flexibench/` | `icelake/parlay/etimeicelake.tex`, `eusericelake.tex`, `esysicelake.tex` | `icelake/parlay/eparlayicelake.bib` | `3e944092514577af02cc714b298e2f70477f37f211de93ee6a59e2c3cb0f7417` |
| stacks, Xeon | `icelake/stacks/.flexibench-stacks/` | `icelake/stacks/ktimeicelake.tex`, `kusericelake.tex`, `ksysicelake.tex` | `icelake/stacks/estacksicelake.bib` | `cafed96beebd2f895ca28f07be08b612946f55696fe9267c4e76c71472b77d3c` |
| patience, Xeon | the store of the programs | `icelake/parlay/ptimeicelake.tex`, `pusericelake.tex`, `psysicelake.tex` | `icelake/parlay/eparlayicelake.bib` | `459e95bff24c97c4fc18180579c769e5c6b662d47e0c8cf1ab0e39a5c3c1d549` |

Each view also exists as `.html`. The M5's certificates were issued by Flexibench 2a10c0c
and the Ryzen's and the Xeon's by Flexibench 27e6e1e, all with Lean 4.30.0. The `.bib` files are
written by hand, so `check-view` doesn't check them. Each round also has a certificate
of its analysis `failures`, which lists the runs that failed or printed a wrong result;
it is empty in all seven rounds (on the Xeon,
`f85cb2d37ae97c982e53b13a06c53065fef873900bdc2bfa3f9c710d5ed22f63` for the stacks round, `11976f9b14a86f984226d9244889a8a79c1a64c1bf66798f2660e8b4090fe1bb` for `fib` and
`664b66bb3c077c94ef4fad07cd116bce086b0db89c8d1d934325efc2d2480d1c` for the programs).

## The builds

| Name | Commit | What it is |
|---|---|---|
| `plain` | 5041c96 (branch `perf-slots-direct`) | `doc/native-fork-join/microtaskparts`: never sleeps |
| `elastic` | a8a1d99 (branch `perf-elastic`) | `doc/elastic-scheduling/microtaskparts`, the micro taskparts of this directory's parent |
| `ordered` | 505b4a8 (branch `perf-elastic-ordered`) | `elastic`, except that `push` orders its own store on every machine and nothing calls `membarrier`. Ryzen and Xeon (`fib` only there); on AArch64 it is `elastic` |
| `alpha` | 81aacec (branch `perf-elastic-alpha`) | `elastic`, except that a thief that has stolen wakes up to two sleepers, whether or not another worker is stealing. Xeon only. The paper doesn't use its values |
| `plain2` | 979f3c6 (branch `perf-plain-stacks`) | `doc/native-fork-join/microtaskparts` as it is now: each worker keeps up to 16 stacks of its own for the tasks it steals, and uses the shared pool, under its lock, only beyond that. Stacks round only |
| `elastic2` | bb22c43 (branch `perf-elastic-stacks`) | `elastic` with the same change. Stacks round only |
| `serial` | ParlayLib 80bf19d (1e1d0b8 on the Xeon) | ParlayLib with no scheduler (`PARLAY_SEQUENTIAL`) |
| `homegrown` | ParlayLib 80bf19d (1e1d0b8 on the Xeon) | ParlayLib's own scheduler |

The six branches were made only for measurements. The micro taskparts of this
directory's parent is that of bb22c43, apart from two comments. `plain` and `elastic`
share one pool of stacks among all workers; the paper says which of its measurements
are of them: the Xeon's `fib`, programs and patience rounds. Its Table 4
uses `plain2` and `elastic2`.

The M5's and the Ryzen's rounds use the builds with stacks for each worker, under the
names the clients have: `plain` is 979f3c6, `elastic` bb22c43, and `ordered` 044dbe7
(branch `perf-elastic-ordered-stacks`: bb22c43 with the push ordering its own store).
They replace rounds of 2026-11-19 that used 5041c96, a8a1d99 and 505b4a8, whose stores
remain on gamora in the measurement repository's `runs/tbl-gamora-elastic-fib` and
`-parlay`. Their failures certificates are
`09583855d537fc040ab4bd2b3d211fe37cb9cd69eb2a58c46030435dd381c360` and
`e6f8b2b78891ff1458dc8062eb2b469141452f16aebbf7977e7cf6f4a4fe2684`. Transparent huge
pages were at `madvise`, the Ryzen's default. The M5's failures certificates are
`399bd49467274812e881579bd56cbf1928cf60083385b7de45f4232d06feb538` and
`4c5a0489d5e73cd4506560710992b6214814751074205c64bee0ba2ae980cf00`; its earlier stores
are not kept here. The M5's rounds ran while the laptop was in use, each after four
consecutive samples at 93% idle or better; its times vary more than the others'.
ParlayLib is github.com/mikerainey/parlaylib, unmodified. `bursty` is
`doc/elastic-scheduling/parlay/bursty.cpp` at a8a1d99, compiled as one more ParlayLib
example. The `fib` builds use `-O2 -falign-functions=64`, and the ParlayLib builds
CMake's Release flags, `-O3 -DNDEBUG`, with `-falign-functions=64`. The compiler is
Apple clang 21.0.0 on the M5 and clang 21.1.8 on the Ryzen and on the Xeon.

## The `fib` rounds

`fib 38` with each build, five runs each, the runs of the builds interleaved; a value is
the median of the five. Workers: 1, 4 and 10 on the M5; 1, 2, 4, 8, 16 and 32 on the
Ryzen. `\fbval{efib}{elastic}{P = 4}` and `\fbval{efibgamora}{ordered}{P = 1}` print
values.

The rounds also checked the elastic builds with `fib 27`, `migrate 200000 3` and the
`bursty 30 40` of the micro taskparts directory, twice each on 2, 8 and 16 workers (3, 16
and 64 on the Ryzen), with the default patience and with `TASKPARTS_ELASTIC_PATIENCE=1`,
which makes a worker try to sleep after every failed steal. Every check passed.

## The rounds of programs

Five programs, each with `serial`, and with `plain`, `elastic` and `homegrown` on 1, 4
and 10 workers (1, 4, 16 and 32 on the Ryzen): `mergesort` of 20 million integers,
`samplesort` of 50 million, `quickhull` of 50 million points, `BFS` of a graph with 2
million vertices, and `bursty 38 10`. A program runs its algorithm 5 times (the sorts)
or 3 times in one process, and ParlayLib's timer writes each run's elapsed time and the
process's user and system time over that run. Each program and build ran as five
processes, interleaved. A value of `etime`, `euser` or `esys` is the median over the
five processes of the median of a process's runs. The paper's burn is the sum of the
`euser` and `esys` values, computed in `main.tex`.

Every run's printed result was checked against the one the `serial` build prints. The
rounds also ran `elastic` and `homegrown` on 2, 8 and 16 workers (3 and 64 on the Ryzen)
for that check alone. Every check passed.

On the Ryzen the frequency governor was `performance`, with frequency boost on and both
hardware threads of each core enabled. The M5's rounds each started after four
consecutive samples at 93% idle or better; the laptop was in interactive use that
evening, and its times vary more than the Ryzen's.

The clients are `mtpelasticfib` and `mtpelasticparlay` of the measurement repository
(run with `--ryzen` on the Ryzen), and the builds are made by its `scripts/build-mtp-dir`
and `scripts/build-elastic-bench`.

## The Xeon's rounds

Both rounds ran every process under `numactl --interleave=all`, with no pinning, the
frequency governor `performance`, automatic NUMA balancing off, and turbo and both
hardware threads of each core on. Sequential and one-worker configurations ran as one
process each.

**`fib`.** `fib 45` with `plain`, `elastic`, `alpha` and `ordered`: five runs each on 40,
80 and 160 workers, interleaved, a value being the median of the five, and one run each
on 1 worker. `\fbval{efibicelake}{ordered}{P = 1}` prints a value. The round also checked
`elastic`, `alpha` and `ordered` with `fib 27`, `migrate 200000 3` and `bursty 30 40`,
twice each on 3, 80 and 320 workers, with the default patience and with patience 1.
Every check passed.

**Programs.** Nine programs, each with `serial`, and with `plain`, `elastic`, `alpha` and
`homegrown` on 40, 80 and 160 workers as three processes each, interleaved; `plain`,
`elastic` and `homegrown` also ran on 1 worker, once.

| Row | Program and input |
|---|---|
| `mergesort` | 32,768,000 integers |
| `samplesort` | 32,768,000 integers |
| `quickhull` | 131,072,000 points |
| `BFS` | `rmat-23.adj` |
| `scan` | n = 524,288,000 |
| `kcore` | `rmat-24.adj` |
| `bigint_add` | n = 33,554,432,000 |
| `bursty` | `bursty 38 10` |
| `burstylong` | `bursty 42 10`, the same source ("bursty, long" in the paper) |

Values are computed as in the other rounds of programs, over three processes instead of
five. Every run's printed result was checked against the `serial` build's of the same
round, and `elastic`, `alpha` and `homegrown` also ran on 3 and 320 workers for that
check alone. Every check passed.

**Patience.** In the same round, `bursty` and `burstylong` ran with `elastic` on 80 and
160 workers with `TASKPARTS_ELASTIC_PATIENCE` set to 100, 10000 and 100000, three
processes each. The views `ptimeicelake`, `pusericelake` and `psysicelake` hold them, with
configurations such as `100 on 80`. The paper's column for patience 1000, the default,
comes from the `elastic 80` and `elastic 160` configurations of the main views.

**Stacks.** A later round the same day, in `icelake/stacks/`: the nine programs on 80
workers with `plain`, `elastic`, `plain2`, `elastic2` and `homegrown`, three processes
each, interleaved, and `serial` once; same settings, same checks, all passed. The views
`ktimeicelake`, `kusericelake` and `ksysicelake` have the configurations `serial`,
`plain 80`, `elastic 80`, `plain2 80`, `elastic2 80` and `homegrown 80`. The paper's
Table 4 is `plain2`, `elastic2` and `homegrown`. Run with `--stacks`, the client keeps
its store in `.flexibench-stacks`, so the Flexibench commands below need that store
named there.

The clients are `mtpelasticfibicelake` and `mtpelasticparlayicelake` of the measurement
repository.

## The collection on the Ryzen

`gamora/suite` holds a round of 20 ParlayLib examples and `bursty` on the Ryzen
(2026-11-23), made by the client `MtpElasticSuite` of the measurement repository, which
`suite-elastic/gen-client.py` writes from `suite-elastic/programs-ryzen.txt`. Five
builds, all at ParlayLib 1e1d0b8:

| build | scheduler | taskparts commit |
|---|---|---|
| `serial` | none (`PARLAY_SEQUENTIAL`) | |
| `plain` | never sleeps: `doc/native-fork-join/microtaskparts` | 979f3c6 |
| `elastic` | `doc/elastic-scheduling/microtaskparts` | 77e1198 |
| `homegrown` | ParlayLib's own | |
| `stats` | `elastic` with `-DMTP_STATS`, and ParlayLib's timer with `parlay/timer-regions.patch` | 77e1198 |

77e1198 (the local branch `perf-elastic-instr`) is bb22c43 plus the instrumentation of the
paper's appendix. The serial build and the one-worker runs ran once, the three schedulers
on 16 and 32 workers and `stats` on 4, 8, 16, 24 and 32 three times each; 966 processes,
none failed. Certificates:

- `times` (views `stime`, `suser`, `ssys`): 44e59981b8fc1cf4fb4c90ecce199df18312d83b49c4b9c925cdead8b7631727
- `totals` (views `fwork`, `fsched`, `fidle`, `fasleep`, `fsteals`, `fsleeps`, `fcancels`,
  `fforkwakes`, `fthiefwakes`): 6877341643c75dff55276d3828e9ad1c081fc9e242091143acd6894e1723a1af
- `speedups` (view `sspeedup`): f209d2da9bc1201a61fc7526f3982c28509632405685d75612b0bb7edad09177
- `failures`, empty: 6b79e1a727cff24015b6a4861e1a4f0d70263e27344f29588379b209643a931b

The `times` and `failures` certificates were made on the Ryzen; the other two, and the
views of `totals` and `speedups`, were made on the laptop from a copy of the store, with
the same client. Only five of the views have an `.html` beside the `.tex`.

## The Xeon again (2026-11-23)

A second lease on the same kind of node. Its script is `runs/run-icelake-elastic2.sh` of
the measurement repository. Builds: `plain` 979f3c6, `elastic` and `stats` 77e1198,
`ordered` 044dbe7; ParlayLib 1e1d0b8. These replace, in the paper, the rounds of
`icelake/fib` and the patience sweep of `icelake/parlay`, which measured micro taskparts
with one shared pool of stacks; those directories are kept.

- `icelake/fib2` (store `.flexibench-stacks`, client `mtpelasticfibicelake --stacks`):
  `fib(45)` with `plain`, `elastic` and `ordered` on 1, 40, 80 and 160 workers. View
  `kfibicelake`. Certificates: `times`
  6b21890ba7b65a1fb7c887d51fcc637f0fd36fc0ab41e23a97b6f783a897a294, `failures` (empty)
  299c5646f8b66e5415faa7377bb77e5d07ea758d2a2330eec460eaea916f9242.
- `icelake/patience2` (store `.flexibench-patience`, client
  `mtpelasticsuiteicelake --patience`): `bursty` at two sizes with `elastic` and `stats`
  on 80 and 160 workers, with a patience of 100, 1000, 10000 and 100000. Views `qtime`,
  `quser`, `qsys` (from `elastic`) and `qidle`, `qasleep`, `qsleeps` (from `stats`).
  Certificates: `patience`
  ef9adf693efe2a6434bdee75ce8218afdeb7932dab33ebfe0d2b23462856cf83, `failures` (empty)
  85a3688f8de56937352363e76d37d5d2d570834bf367bdcd927e5083052f6e82.
- `icelake/suite` (store `.flexibench`, client `mtpelasticsuiteicelake`, written by
  `suite-elastic/gen-client.py` from `suite-elastic/programs-icelake.txt`): the
  collection of `gamora/suite` at the sizes of the fork join paper's suite on the Xeon.
  The serial build and `stats` on one worker ran once; `plain`, `elastic` and
  `homegrown` on 40, 80 and 160 workers and `stats` on 20, 40, 80, 120 and 160 three
  times each; 1008 processes in 97 minutes, none failed. In the paper it also replaces
  the last use of `icelake/parlay` (bursty on 160 workers), which is kept. Certificates:
  - `times` (views `stime`, `suser`, `ssys`): 4fd1b8c8ee3d42cb96cdc3acc3984263a1a415c74c5f552758d1197134541b46
  - `totals` (views `fwork`, `fsched`, `fidle`, `fasleep`, `fsteals`, `fsleeps`,
    `fcancels`, `fforkwakes`, `fthiefwakes`): 8d3418594da573502aa7d989faf64162b12469a349b68ec9f2579b85c17685b6
  - `speedups` (view `sspeedup`): b523b4c1ad072b36726640aa980e94bf0c30b22a3ba7950a689e51874bef11cf
  - `failures`, empty: 4d568b941c0597d6ed7e31a268ec85a0c66f80acc0e0ff9cb7f4a048964e3ba3

  The `failures` certificate was made on the node; the others and the views, on the
  laptop from a copy of the store, with the same client. The views have no `.html`.
- `icelake/thp` (store `.flexibench-thp`, client `mtpelasticsuiteicelake --thp`): `scan`,
  `bigint_add`, `filter` and `flatten` at the same sizes with `serial`, and `plain`,
  `elastic` and `homegrown` on 80 workers, with
  `/sys/kernel/mm/transparent_hugepage/enabled` at `always` (every other round ran
  under `madvise`, the node's default). The step reads the setting and refuses to run
  unless it is `always`; it does not write it. Views `htime`, `huser`, `hsys`.
  Certificates: `times` 63483c75ffa30d05d8adf6590c3b2d699de5e8b591f141e194c18ccf8d787732,
  `failures` (empty) bc1f584e89191a9baa7697b8ac41652aafd83acc5bca201e468043aa3e5f0914.

## Stores not in the repository

Four stores are too large to commit and are ignored by the repository's `.gitignore`:
`gamora/suite/.flexibench`, `icelake/suite/.flexibench`,
`icelake/stacks/.flexibench-stacks` and `icelake/parlay/.flexibench` (about 200, 195, 66
and 67 MB). Their views and certificates are here; checking a view of one of them needs
a copy of its store.

## Checking

From each store directory, with the Flexibench that issued its
certificate:

    flexibench check-objects
    flexibench check <certificate>
    flexibench check-view <view>.tex
    flexibench check-view <view>.html

## Not certified

On the Xeon, before the stacks round, `perf` profiles of `kcore` on 80 workers showed
`plain` spending about 24% of its processor time in the `futex` system call and 34% in
the kernel's spinlock slow path; `plain` has no lock but the stack pool's. The stacks
round certifies the effect on time: 6.47 s with the pool, 1.70 s without. The profiles
are in the measurement repository's `runs/icelake-elastic/node/diag/`.

Before these rounds, runs by hand on the Ryzen timed `fib 38` with an earlier `elastic`
whose `push` used a sequentially consistent store on x86-64: 0.75 s on one worker against
0.57 s for `plain`. That finding led to the remote barrier, and the `ordered` build
reproduces it in the certified round.
