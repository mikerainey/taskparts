# Measurements for the paper

Table `tbl:cost` of `../main.tex` cites the values in `times.tex`. Flexibench generated
that file from the measurements recorded in the store `.flexibench/`.

- `.flexibench/`: the Flexibench store of one round, measured on 2026-11-11 on the
  Apple M5 described in the paper. It holds:
  - `fib 38` with six builds of the instrumented micro taskparts, on 1, 4 and 8 workers,
    with five runs of each and the runs of the six builds interleaved;
  - correctness checks of every build, all of which passed: `fib 27`; `migrate`, for the
    four builds that have it; and, for the three builds that count forks, that `fib 27`
    counts exactly 317810;
  - the stats line that each instrumented run of `fib` printed (work, scheduling and idle
    time, steals, forks). No view shows these yet.
- `times.tex`: the view `times`, the median time of each build at each worker count, as
  LaTeX macros. After `\input{perf/times}`, `\fbval{times}{stats}{P = 4}` prints a value
  and `\fbcertificate{times}` prints the certificate's digest.
- `times.html`: the same view, as a web page.
- `times.bib`: the paper's citation of the view, `flexibench-times`, which gives the
  certificate's digest. It is written by hand for now, so `check-view` doesn't check it.

The view is certified by certificate
`c0fb867f2833487f3726929cd7f3d353e757e6d6cdd39837336ad595a5c348fc`, issued by Flexibench
2a10c0c with Lean 4.30.0.

## The builds

All six are builds of `doc/instrumentation/microtaskparts` at commit ea6814e, on the
branch `perf-instr`, with `-O2 -falign-functions=64`.

| Row | Flag | What it is |
|---|---|---|
| `plain` | none | no instrumentation |
| `stats` | `-DMTP_STATS` | time per worker in each state, and steals |
| `log` | `-DMTP_LOG` | `stats`, plus the log of changes of state and steals |
| `forks` | `-DMTP_COUNT_FORKS` | `stats`, plus a count of forks in each worker's recorder |
| `atomic` | `-DCOUNT_FORKS_ATOMIC` | `fib` only: `plain`, plus a count of forks in one shared atomic counter |
| `packed` | `-DCOUNT_FORKS_PACKED` | `fib` only: `plain`, plus a count of forks in per-worker counters packed in one array |

The client that ran the round is `mtpinstr`, in the taskparts-bench repository
(`client/MtpInstr.lean`, `scripts/build-mtp-instr`, `runs/run-tbl-instr.sh`).

## Checking

From this directory, with Flexibench 2a10c0c:

    flexibench check-objects
    flexibench check c0fb867f2833487f3726929cd7f3d353e757e6d6cdd39837336ad595a5c348fc
    flexibench check-view times.tex
    flexibench check-view times.html

These commands confirm three things:
- every object in the store has the digest it is stored under;
- the certificate holds for the store's log;
- each view shows exactly what the certificate certifies.

They don't check how the table in the paper is laid out around the values.
