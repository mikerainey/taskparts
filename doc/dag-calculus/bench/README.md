# More benchmarks for DAG calculus

Programs that exercise the operations of DAG calculus (`new_vertex`, `new_edge`,
`release`, `wait`) in the micro taskparts of this paper
(`../microtaskparts/microtaskparts.hpp`), beyond the pipeline and the stencil that
the paper presents. They are ports, simplified, of benchmarks from three sources:

- Lee et al., *On-the-fly pipeline parallelism* (SPAA 2013), the Cilk-P paper,
  whose scheduler is Piper;
- Tang et al., *The Pochoir stencil compiler* (SPAA 2011);
- the examples and design patterns of Intel's Threading Building Blocks (TBB).

Nothing here is cited by the paper, and nothing here has been measured in a
certified round. The scheduler is used as it is: no file of `../microtaskparts`
was changed.

## The programs

Each program has a serial version, one or more fork join versions where fork
join can express the computation, and a `dag` version. Every version of a program
calls the same kernel, compiled once and not inlined, so the versions differ
only in how the work is scheduled.

| program | from | versions | what the `dag` version exercises |
|---|---|---|---|
| `pipefib` | Cilk-P: pipe-fib, pipe-fib-256 | `serial`, `dag` | a pipeline that is a grid of nodes, each waiting for the node beside it in the previous iteration; with one bit per stage, the cost of a node |
| `ferret` | Cilk-P: ferret (PARSEC) | `serial`, `dag` | a serial, parallel, serial pipeline with nearly all work in the parallel stage |
| `x264` | Cilk-P: x264 (PARSEC) | `serial`, `dag` | a pipeline whose nodes and edges are decided as it runs: stages are skipped, an iteration waits for a stage `reach` rows ahead in the previous one, and fork join runs inside an iteration |
| `heat` | Pochoir: Heat (2D) | `loops`, `ploops`, `trap`, `dag` | vertices made, linked, released and waited for at every level of a recursion |
| `life` | Pochoir: Life | the same | the same, with a nine-point stencil over bytes |
| `lcs` | Pochoir: LCS; TBB: the wavefront pattern | `serial`, `planes`, `dc`, `dag` | a grid of vertices with implicit out-edges and in-degree two |
| `cholesky` | TBB: the Cholesky flow graph; KaStORS | `serial`, `levels`, `dag` | a graph built with explicit edges in parallel, while its first vertices already run, so that some edges arrive after their source has finished |
| `preorder` | TBB: `parallel_preorder` | `serial`, `levels`, `dag` | a large random graph of small vertices: the cost of a vertex and of an edge |

The pipeline of the paper (`../microtaskparts/pipeline.cpp`) is the fourth
benchmark of the Cilk-P paper, dedup.

### What stands in for what

- **The pipelines** share `pipe.hpp`, a `pipe_while` with the stage operations of
  Cilk-P (`s.next(j)` for `pipe_stage`, `s.wait(j)` for `pipe_stage_wait`) and its
  throttling, written with the four operations of DAG calculus and nothing of the
  scheduler's own. An iteration is a vertex. An iteration that must wait for the
  previous one makes a vertex with an empty body and waits for it, and the
  previous iteration releases that vertex when it enters a later stage. An
  iteration that finds the previous one far enough along makes no vertex.
- **The inputs are synthetic.** `ferret` ranks made-up images against a made-up
  database by scanning all of it. `x264` encodes a made-up video with one
  motion search per macroblock and no transform or entropy coder. What is kept
  from the originals is the structure of the dependencies and where the work is.
- **The stencils** share `stencil2d.hpp`, the trapezoidal decomposition of Pochoir
  with hyperspace cuts, for two dimensions and a boundary that is held fixed
  (Pochoir also handles periodic boundaries). `trap` runs the zoids of a cut in
  three rounds by fork join, as Pochoir does. `dag` makes each zoid of a cut a
  vertex with an edge from exactly the zoids that it reads.
- **`cholesky`** uses plain loops for its three kernels, where the originals call
  BLAS and LAPACK.
- **`preorder`** uses matrices of integers that wrap around, so its result is
  exact.

### Not ported

- From Pochoir: the three-dimensional stencils (Wave, LBM), and PSA, RNA and
  APOP. `stencil2d.hpp` handles two dimensions only.
- From TBB: `fgbzip2`, a pipeline of the same shape as dedup.

## Building and testing

    make                 # bin/NAME for each program
    make bin/lcs.cnt     # with counts of the operations of DAG calculus
    ./test.sh            # every program, small inputs, three builds for testing

Each program prints its usage when given a wrong argument. With `check` as its
last argument, a program compares its result with that of its serial version,
bit for bit, and prints `check ok`. `pipefib` checks against the same recurrence
computed modulo a prime instead, and `cholesky` also checks that the factor
multiplies back to the matrix.

`test.sh` runs the builds of `../microtaskparts/Makefile` for testing: with
pauses at the scheduler's races and checks of stacks and of vertices that
nothing waited for, and the same under the address and thread sanitizers, on 1,
2, 3, 4, 10 and 16 workers.

## A first look, not a measurement

One run of each version on the M5, on 1 and on 10 workers, on 2026-11-21, to see
that the programs scale. These are single runs outside Flexibench: nothing here
may be cited. Times in seconds; "serial" is the version that starts no thread.
The sizes were smaller than today's defaults, and are given so that a run can
be repeated.

| program and arguments | serial | version | 1 worker | 10 workers |
|---|---|---|---|---|
| `pipefib` 30000, 256 bits per stage | 0.233 | `dag` | 0.217 | 0.068 |
| `pipefib` 30000, 1 bit per stage | 0.503 | `dag` | 0.990 | 0.541 |
| `ferret` 3000 20000 | 0.653 | `dag` | 0.648 | 0.122 |
| `x264` 60 640 368 2 | 0.863 | `dag` | 0.871 | 0.161 |
| `heat` 2000 400 | 0.436 | `ploops` | 0.441 | 0.209 |
| | | `trap` | 0.402 | 0.097 |
| | | `dag` | 0.402 | 0.087 |
| `life` 2000 400 | 0.469 | `ploops` | 0.466 | 0.071 |
| | | `trap` | 0.579 | 0.085 |
| | | `dag` | 0.582 | 0.084 |
| `lcs` 60000 512 | 1.558 | `planes` | 1.579 | 0.254 |
| | | `dc` | 1.585 | 0.442 |
| | | `dag` | 1.579 | 0.233 |
| `cholesky` 2048 64 | 0.299 | `levels` | 0.291 | 0.056 |
| | | `dag` | 0.285 | 0.052 |
| `preorder` 200000 | 0.094 | `levels` | 0.101 | 0.017 |
| | | `dag` | 0.099 | 0.020 |

What these single runs suggest, to be confirmed or not by real rounds:

- With one bit per stage, `pipefib` on one worker takes about twice the serial
  time, and ten workers only bring it back to the serial time. Lee et al. report
  the same of pipe-fib: its nodes are too small to pay for a pipeline.
- On `lcs`, the divide and conquer of fork join is the slowest version on ten
  workers, and the `dag` version is about as fast as the antidiagonals.
- On the stencils and on `cholesky`, the `dag` version is close to the fork join
  version that runs the same tasks in rounds.
