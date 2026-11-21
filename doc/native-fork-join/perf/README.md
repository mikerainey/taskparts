# Measurements for the paper

The tables of `../main.tex` cite values that Flexibench generated from the measurements
recorded here. There are eight rounds, each in a store of its own, all measured on
2026-11-20: on each of two machines, the cost of a fork, four ParlayLib programs, the
ParlayLib examples as a whole, and one more example, `karatsuba`, in a short round of
its own. The first two tables are from the Apple M5 described in the paper.

The paper names the machines by their processors (its Table `tbl:machines`): the M5 is
the laptop, and the Ryzen is the machine called gamora here and in the file names.

| Table | Store | Values | Citation | Certificate |
|---|---|---|---|---|
| `tbl:perf` | `.flexibench/` | `times.tex`, `times.html` | `times.bib` | `61f60429d4723b41fa7e3855457010fa6d21e7b35b4da5feb659b16fc882fe41` |
| `tbl:parlay` | `parlay/.flexibench/` | `parlay/parlay.tex`, `parlay/parlay.html` | `parlay/parlay.bib` | `d8890a98eef929ae9d9e09aacf2f098ba1947a0991d7a7d96683b9fe2c8e91f2` |

Both certificates were issued by Flexibench 2a10c0c with Lean 4.30.0. The two `.bib`
files are written by hand for now, so `check-view` doesn't check them.

After `\input{perf/times}`, `\fbval{times}{direct}{P = 4}` prints a value of the first
table, and after `\input{perf/parlay/parlay}`, `\fbval{parlay}{BFS}{direct 4}` one of
the second.

## Table `tbl:perf`: the cost of a fork

`fib 38` with three builds of `doc/native-fork-join/microtaskparts`, on 1, 4, 8 and 10
workers, with five runs of each and the runs of the three builds interleaved. Each value
is the median of the five. The round also checked every build with `fib 27` and
`migrate`; every check passed.

| Row | Commit | What it is |
|---|---|---|
| `serial` | 67a207b | `fork2join` runs its two branches as two calls, with no scheduler |
| `locked` | a66c905 | the paper's algorithm, with each deque protected by a lock |
| `direct` | 99d3b71 | the paper's algorithm, with the lock-free deque: micro taskparts as it is |

Commits 67a207b and a66c905 were made only for this table. They are on the branches
`perf-cost-serial` and `perf-cache-locked`. Commit 99d3b71, on the branch
`perf-cache-direct`, has the same micro taskparts as this directory's parent: each
worker keeps up to 16 released stacks of its own, in front of the shared pool.
The `serial` build ignores the
number of workers, and the paper's table shows it for one worker only.

## Table `tbl:parlay`: programs that work between forks

Four example programs of ParlayLib (github.com/mikerainey/parlaylib, commit 80bf19d,
unmodified), each with three builds:

| Column | What it is |
|---|---|
| `serial` | ParlayLib with no scheduler (`PARLAY_SEQUENTIAL`) |
| `direct 1`, `direct 4`, `direct 10` | ParlayLib on micro taskparts at 99d3b71, through `doc/parlay-adapter`, on 1, 4 and 10 workers |
| `homegrown 1`, `homegrown 4`, `homegrown 10` | ParlayLib's own scheduler, on 1, 4 and 10 workers |

The programs and their inputs: `mergesort` of 20 million integers, `samplesort` of 50
million, `quickhull` of 50 million points, and `BFS` of a generated graph with 2 million
vertices. Each program generates its input, then runs its algorithm 5 times (the sorts)
or 3 times in one process. Each program and build ran as five processes, interleaved.
A value is the median over the five processes of the median of a process's runs, its
first run included.

Every run's printed result was checked against the result the `serial` build prints.
The round also ran both schedulers on 2, 8 and 16 workers for that check alone. Every
check passed.

All builds use CMake's Release flags, `-O3 -DNDEBUG`, with `-falign-functions=64`.

## Table `tbl:second`: a second machine

Both experiments again, on gamora: an AMD Ryzen 9 3950X with 16 cores and 32 hardware
threads, with Linux 6.12 (NixOS) and Clang 21. Measured on 2026-11-20, in one session,
with the frequency governor set to `performance`, frequency boost on, and both hardware
threads of each core enabled. The paper's table shows 1 and 16 workers; the views also
hold 2, 4, 8 and 32.

| Store | Values | Citation | Certificate |
|---|---|---|---|
| `gamora/.flexibench/` | `gamora/timesgamora.tex`, `.html` | `gamora/timesgamora.bib` | `ef38473d89210db331f9697de05ae6a26015d4d4769b23a827311989b17af62a` |
| `gamora/parlay/.flexibench/` | `gamora/parlay/parlaygamora.tex`, `.html` | `gamora/parlay/parlaygamora.bib` | `b80a1d20645b3d189442cb92810478294606c3457fc92e13c4419a097e511266` |

Both certificates were issued on gamora by Flexibench 27e6e1e, the first revision that
builds on Linux, with Lean 4.30.0. `\fbval{timesgamora}{direct}{P = 16}` and
`\fbval{parlaygamora}{BFS}{direct 16}` print values of the two views.

The builds are those of the first two tables, compiled on gamora, with one difference:
they were compiled there. The programs and their input sizes are the same,
but ParlayLib's generators give other inputs on Linux than on macOS, so the results the
round checked against are the ones the `serial` build prints on Linux. The round ran
each program and build as five processes, and checked both schedulers on 3 and 64
workers as well. Every check passed.

The clients are `mtpperfgamora` and `mtpparlaygamora`, on the branch `gamora-nfj` of
the measurement repository.

## Tables `tbl:suite` and `tbl:suitegamora`: the ParlayLib examples as a whole

60 of the 63 example programs of ParlayLib, each with one input, on the three builds of
`tbl:parlay`. Measured on 2026-11-20: on each machine, one round of 59 programs, and a
short one of `karatsuba` alone (below).

| Table | Store | Values | Citation | Certificate |
|---|---|---|---|---|
| `tbl:suite` (M5) | `suite/.flexibench/` | `suite/suite.tex`, `.html` | `suite/suite.bib` | `3b1a693a01f35b76e6b3a2e50af5beab345f6de7fe019624d9ff29ab09f22f51` |
| its row `karatsuba` | `suite/karatsuba/.flexibench/` | `suite/karatsuba/suitek.tex`, `.html` | `suite/karatsuba/suitek.bib` | `96b1496786d4bbd2f5cd2b1d1f54682b4cf3861c589f99a6309d4a178fea75d9` |
| `tbl:suitegamora` (Ryzen) | `gamora/suite/.flexibench/` | `gamora/suite/suitegamora.tex`, `.html` | `gamora/suite/suitegamora.bib` | `48324ab8e8583666bc0c78cb46aa95b5e4bbd98983b4d2ec7c089fbc285d7373` |
| its row `karatsuba` | `gamora/suite/karatsuba/.flexibench/` | `gamora/suite/karatsuba/suitekgamora.tex`, `.html` | `gamora/suite/karatsuba/suitekgamora.bib` | `ddd5628d220a0773692556cc6aa94bc85d11ca9bd9e7e501677460cf8a88132d` |

The M5's certificates were issued by Flexibench 2a10c0c, the Ryzen's on gamora by
Flexibench 27e6e1e, all with Lean 4.30.0. `\fbval{suite}{kcore}{direct 10}` and
`\fbval{suitegamora}{kcore}{direct 32}` print values of the two views. The columns are
those of `tbl:parlay`, on 1, 4 and 10 workers on the M5 and on 1, 16 and 32 on the Ryzen.

The two stores of 59 programs are large (259 MB and 279 MB): a store holds every
executable that ran, and the round has 177 of them. For that reason they are **not in
this repository**; their views and citations are. The stores are in the measurement
repository's working copies, `runs/tbl-cache-suite2/.flexibench` on the laptop and
`runs/tbl-gamora-cache-suite/.flexibench` on gamora. The checks of these two rounds
(below) need a store copied to `suite/.flexibench` or `gamora/suite/.flexibench`.
The two stores of `karatsuba` are here.

**Builds.** ParlayLib at commit 783e811 of github.com/mikerainey/parlaylib (branch
`microtaskparts`), micro taskparts at 99d3b71, through `doc/parlay-adapter`, with the
flags of the other tables. Against the commit of `tbl:parlay`, 80bf19d, ParlayLib
differs in four ways:

- its timer writes the file of recorded times only from a timer that recorded one.
  Before, every timer rewrote the file when it was destroyed, and the library makes
  timers inside some of its primitives, which slowed down the programs that use those
  primitives whenever times were being recorded. The four programs of `tbl:parlay` are
  not among them: with and without recording, on 1 and 32 workers, their times agree;
- `find_if` times its search, and searches for a value that is not there, so that every
  run reads the whole input;
- `push_relabel_max_flow` times its algorithm as a whole;
- a new program, `gen_rmat_graph`, writes a graph to a file.

**Inputs.** A program takes a size, or reads a file. The files are made once and every
run reads them; they are not in this repository. `suite/SHA256SUMS` lists
their digests, which are the same on both machines.

- The graphs `rmat-18.adj` … `rmat-24.adj` are symmetric R-MAT graphs with 2^18 … 2^24
  vertices and about 20 edge endpoints per vertex, written by `gen_rmat_graph 2^k FILE`.
  The generator gives the same graph on Linux and on macOS.
- The texts are from the Problem Based Benchmark Suite
  (github.com/cmuparlay/pbbsbench, `testData/data`): `chr22.dna` (35 MB), `etext99`
  (105 MB), `wikisamp.xml` (100 MB), `wikipedia250M.txt` (250 MB).

A graph program that generates its own graph spends 65% to 97% of a run on one core
generating it, which is why the graphs are stored.

**Sizes.** For each program, the size was doubled until one timed run of the build with
no scheduler took at least 1 s on the Ryzen, or the process came to use a quarter of
the machine's memory (8 GB). The same sizes are used on the M5, whose cores are faster.
Sizes are 1000 times a power of two because the doubling started from 1000. Under 1 s
on the Ryzen, stopped by memory: `reduce`, `find_if`, `flatten`, `linefit`. Under 1 s
because no larger text is at hand: `radix_tree`, `knuth_morris_pratt`. The table of
programs and arguments is `suite/programs-ryzen.txt` in the measurement repository,
and the tables of the paper show the arguments.

**`karatsuba`.** At 783e811, the addition it uses (`examples/bigint_add.h`, the
sequential branch) reads the shorter number beyond its end, and the program sometimes
crashes with micro taskparts on 16 workers or more. Commit 1e1d0b8 of the same branch
reads through the accessor that the parallel branch uses (`B(i)` in place of `b[i]`).
`karatsuba` was measured at 1e1d0b8, with micro taskparts at 99d3b71, in a round of
its own on each machine, with the method of the other 59, which stay at 783e811: the
two commits differ in that one file, which `karatsuba` alone includes. Its size,
16384000, was chosen by the same rule. It prints no result, so its processes are
checked for success only.

**Left out.**

- `hash_map` and `decision_tree_c45`: their drivers record several times per
  repetition, one for each stage, and the analysis takes a median over a process's
  recorded times.
- `lasso_regression`: we have no input file for it.

**Runs.** Each program and build ran as three processes, interleaved; a process runs
its algorithm 1 to 20 times, as its driver decides. A value is the median over the
three processes of the median of a process's runs. A process may take 15 minutes at
most.

**Checks.** For 56 programs, every process must print one expected line: the last line
that all builds print, on every worker count. The other three (`huffman_tree`,
`longest_common_prefix`, `range_min`) print nothing and must exit with success, as must
`karatsuba`. The
expected lines differ between the machines for the 17 programs whose driver makes a
random input. Besides the timed runs, the rounds ran both schedulers on 3 and 16
workers (M5) and on 3 and 64 (Ryzen) for the check alone. Every check passed: the
analysis `failures` is certified empty on both machines (`86947839…` on the M5,
`4be9477f…` on the Ryzen; for `karatsuba`, `6b916535…` and `63a1c3f2…`).

The M5's round took 1 h 34 min, and the Ryzen's 2 h 51 min. The M5's round started
after four consecutive samples at 93% idle or better; the Ryzen's ran with the
settings of `tbl:second`.

The clients are `mtpsuite` (branch `paper-nfj` of the measurement repository) and
`mtpsuitegamora` (branch `gamora-nfj`), and for `karatsuba`, `mtpsuitek` and
`mtpsuitekgamora`. All are generated by `suite/gen-client.py` from a table of programs
and a file of expected lines.

## Earlier rounds

They are in this directory's history:
- at commit d53301d, the four rounds of the first three tables, of 2026-11-17, before
  each worker kept stacks of its own (builds f0767d3 and 5041c96, where every steal and
  every join of a stolen task took the lock of the shared pool). The times of those
  tables were the same to within 6%, except for `fib 38` on 8 workers on the Ryzen
  (0.07 s then, 0.08 s now) and `BFS` on 10 workers of the M5 with ParlayLib's
  scheduler (0.053 s then, 0.057 s now). The rounds of the 59 examples with build 5041c96, of
  2026-11-20, are not in the history; they are in the measurement repository's working
  copies (`runs/tbl-suite` on the laptop, `runs/tbl-gamora-suite` on gamora), with
  certificates `f32181a2…` (M5) and `1c1cedd0…` (Ryzen). There, `spectral_separator`
  took 53% longer with micro taskparts than with ParlayLib's scheduler on 16 workers of
  the Ryzen and 92% longer on 32, `kcore` 15% and 3% longer, and `graph_color` 24% and
  53% longer. In an experiment that was not certified, the difference followed the
  number of steals of a process: 870, 274 and 295 thousand on 32 workers for these
  three, and no difference under about 60 thousand;
- up to commit 4e3070b, rounds that compared this algorithm with a formulation in which
  every fork passes through the scheduler loop;
- at commit 72b975e, the three builds of `tbl:perf` before `launch` was kept out of
  line. There, the compiler inlined `launch` into `fib`, which made every call of `fib`
  save eight more registers, and `direct` took 0.36 s on one worker;
- at commit 131f377, the same three builds before stolen-task stacks got a protected
  page. The times of `direct` were the same;
- at commit 524b1aa, the laptop's two tables before stacks were pooled and before `push`
  stored the deque's bottom index with release ordering (builds b48599d and 958b792). The
  times were the same to within 5%;
- at commit 524b1aa, the second machine's rounds before stacks were pooled (certificates
  `afbd206a…` and `1ccef155…`). There, every steal mapped a stack and the join of a
  stolen task unmapped it, and on 16 workers `quickhull` took 21% longer and `BFS` 74%
  longer with micro taskparts than with ParlayLib's scheduler;
- at commit 40eaecb, all four rounds before the slots of a deque were left
  uninitialized (build 16d4e28 in place of 5041c96). There, starting the workers wrote
  640 KB for each of them, which took about 10 ms for 32 workers on the Ryzen, and
  `fib 38` took 0.04 s there on 32 workers instead of 0.03 s. The other times were the
  same to within 4%. The laptop's rounds then had no runs on 10 workers;
- at commit d244863, both tables before the destructor of a task was removed. There,
  every return from `fork2join` ran a destructor that freed the stolen task's stack, if
  any, and `direct` took 0.31 s on one worker. Now the stolen path alone frees it.

## Checking

With Flexibench 2a10c0c, from this directory:

    flexibench check-objects
    flexibench check 61f60429d4723b41fa7e3855457010fa6d21e7b35b4da5feb659b16fc882fe41
    flexibench check-view times.tex
    flexibench check-view times.html

and from `parlay/`:

    flexibench check-objects
    flexibench check d8890a98eef929ae9d9e09aacf2f098ba1947a0991d7a7d96683b9fe2c8e91f2
    flexibench check-view parlay.tex
    flexibench check-view parlay.html

and, with Flexibench 27e6e1e, from `gamora/` and from `gamora/parlay/`:

    flexibench check-objects
    flexibench check-view timesgamora.tex      (parlaygamora.tex in gamora/parlay/)

and, for the examples as a whole, with Flexibench 2a10c0c from `suite/` and with
Flexibench 27e6e1e from `gamora/suite/`:

    flexibench check-objects
    flexibench check 3b1a693a01f35b76e6b3a2e50af5beab345f6de7fe019624d9ff29ab09f22f51
    flexibench check-view suite.tex            (in gamora/suite/: the certificate
    flexibench check-view suite.html            48324ab8… and suitegamora.tex, .html)

and likewise from `suite/karatsuba/` (`suitek.tex`) and `gamora/suite/karatsuba/`
(`suitekgamora.tex`).

These commands confirm three things:
- every object in a store has the digest it is stored under;
- the certificate holds for the store's log;
- each view shows exactly what the certificate certifies.

They don't check how the tables in the paper are laid out around the values.

## Rounds that were set aside

A first round of the laptop's two tables with the pooled stacks, earlier on 2026-11-17,
ran while the laptop was in interactive use: the round script checked for an idle machine
only once, at its start. `BFS` on 4 workers took 0.085 s with micro taskparts there,
against 0.073 s with ParlayLib's scheduler, and by hand the difference did not reproduce.
The rounds recorded here started after four consecutive samples at 93% idle or better.
The first round is not in this directory.

On the same day, with build 5041c96, the laptop's two rounds first ran without 10
workers. In the ParlayLib round, `samplesort` on 4 workers took 0.266 s with micro
taskparts against 0.242 s with ParlayLib's scheduler; the laptop was in use during the
round, and by hand both took 0.24 to 0.25 s. A rerun was stopped a third of the way
through, when 10 workers were added to the plan. The rounds recorded here are the ones
with 10 workers, which came next. The others are not in this directory.

On 2026-11-20, the M5's rounds of `tbl:parlay` and of the 59 examples with build
99d3b71 first ran in the afternoon. The builds that had not changed were slower there
than in their earlier rounds: `BFS` of `tbl:parlay` on 10 workers took 0.077 s with
ParlayLib's scheduler against 0.053 s, and in the round of the examples ParlayLib's
scheduler on 10 workers was 6% slower at the median and over 10% slower on 20 programs.
Both rounds were run again that evening, and the reruns are the ones recorded here.
The first runs are not in this directory (certificates `69906eb2…` and `4d22d7db…`).
