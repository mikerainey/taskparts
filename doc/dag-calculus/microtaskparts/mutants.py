#!/usr/bin/env python3
"""Checks that the tests can detect the errors they aim at.

Each mutant below is microtaskparts.hpp with one deliberate error, of the kind
that the safety argument of ../main.tex rules out. For each mutant, this
script builds stress.cpp against it and runs the result. A mutant is "killed"
if some run fails: a wrong result, a hang, a crash, or a sanitizer report. A
mutant that survives every run is an error that the tests cannot see.

    ./mutants.py            all mutants
    ./mutants.py NAME ...   only those

Exits with status 1 if a mutant survives, or if the unmutated code fails.
"""

import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
CXX = os.environ.get("CXX", "c++")
BASE = ["-std=c++17", "-pthread", "-O2", "-g", "-DMTP_PERTURB", "-DMTP_CHECK_STACKS"]
BUILDS = {
    "perturb": [],
    "asan": ["-fsanitize=address", "-fno-omit-frame-pointer"],
    "tsan": ["-fsanitize=thread"],
}
# (build, workers, rounds)
RUNS = [
    ("perturb", 1, 2), ("perturb", 2, 6), ("perturb", 4, 6), ("perturb", 10, 6),
    ("asan", 2, 3), ("asan", 4, 3), ("tsan", 2, 2), ("tsan", 4, 2),
]

# name, what the error is, text to replace, replacement
MUTANTS = [
    ("notify-on-stack",
     "a vertex notifies its successors before it has left its stack",
     "inline void vertex::leave(worker &w) { w.finished = this; }",
     "inline void vertex::leave(worker &w) { (void)w; notify(); }"),
    ("no-own-pop",
     "the scheduler loop steals without first popping its own deque",
     "      t = w.deque.pop();\n    }\n    if ((t == nullptr) && (nb_workers > 1)) {",
     "    }\n    if ((t == nullptr) && (nb_workers > 1)) {"),
    ("any-task-is-mine",
     "a fork runs its second branch whenever its pop returns a task",
     "  if (t == &own) {",
     "  if (t != nullptr) {"),
    ("drop-foreign-task",
     "a fork does not put back the task of another that it popped",
     "  if (t != nullptr) {\n    w.deque.push(t);\n  }",
     "  (void)t;"),
    ("release-before-save",
     "wait releases its suspension before the stack is saved",
     "  w.suspended = &s;",
     "  count_down(&s);"),
    ("join-on-stack",
     "a fork counts itself at the join before it has left its stack",
     "  w.reached = &j;",
     "  if (j.pending.fetch_sub(1) == 1) {\n    return;\n  }\n  perturb();"),
    ("no-rollback",
     "new_edge keeps the count of an edge whose source has finished",
     "      dst->pending.fetch_sub(1, std::memory_order_acq_rel);\n      return false;",
     "      return false;"),
    ("edge-before-count",
     "new_edge adds the edge to the out-set before it counts it",
     "  dst->pending.fetch_add(1, std::memory_order_acq_rel);\n  perturb();\n  auto e",
     "  struct late { vertex *d; ~late() { d->pending.fetch_add(1); } } l{dst};\n  auto e"),
    ("out-set-not-closed",
     "a finished vertex leaves its out-set open",
     "  edge *e = out.exchange(finished_mark(), std::memory_order_acq_rel);",
     "  edge *e = out.exchange(nullptr, std::memory_order_acq_rel);"),
    ("stack-shared",
     "a stack goes back to the cache while a task still runs on it",
     "  stack_taken(stack);",
     "  w.stacks.push_back(stack);"),
    ("ready-too-early",
     "a vertex becomes ready with one predecessor left",
     "  if (v->pending.fetch_sub(1, std::memory_order_acq_rel) == 1) {\n    me().deque.push(v);",
     "  if (v->pending.fetch_sub(1, std::memory_order_acq_rel) == 2) {\n    me().deque.push(v);"),
    ("count-not-atomic",
     "a count is decremented by a load and a store",
     "  if (v->pending.fetch_sub(1, std::memory_order_acq_rel) == 1) {\n    me().deque.push(v);",
     "  int c = v->pending.load();\n  perturb();\n  v->pending.store(c - 1);\n  if (c == 1) {\n    me().deque.push(v);"),
    ("touch-after-notify",
     "the scheduler reads a vertex after notifying its successors",
     "    count_down(target);\n    e = next;\n  }\n}",
     "    count_down(target);\n    e = next;\n  }\n  perturb();\n  if (pending.load() < -1000) {\n    std::abort();\n  }\n}"),
    ("join-resumed-twice",
     "the join resumes its parent when either branch arrives",
     "      if (j->pending.fetch_sub(1, std::memory_order_acq_rel) == 1) {\n        // Both",
     "      if (j->pending.fetch_sub(1, std::memory_order_acq_rel) <= 2) {\n        // Both"),
]

# Mutants that no test can kill, because they are not errors. They stay in the
# list so that a change which makes one of them matter is noticed.
HARMLESS = {
    "notify-on-stack":
        "the scheduler keeps nothing in a vertex while it runs, so successors "
        "may be counted down before the vertex's stack is left",
}


def build(directory, name):
    exe = os.path.join(directory, "stress." + name)
    cmd = [CXX] + BASE + BUILDS[name] + ["-I", directory,
                                         os.path.join(directory, "stress.cpp"), "-o", exe]
    r = subprocess.run(cmd, capture_output=True, text=True)
    return exe if r.returncode == 0 else None


def run(exe, workers, rounds):
    """Returns None if the run passed, and otherwise how it failed."""
    env = dict(os.environ, TASKPARTS_NUM_WORKERS=str(workers), MTP_STRESS_TIMEOUT="20",
               TSAN_OPTIONS="halt_on_error=1", ASAN_OPTIONS="detect_leaks=0")
    try:
        r = subprocess.run([exe, str(rounds), "11"], capture_output=True, text=True,
                           env=env, timeout=240)
    except subprocess.TimeoutExpired:
        return "hang (killed)"
    if r.returncode == 0:
        return None
    out = r.stdout + r.stderr
    if "HANG" in out:
        return "hang"
    if "WRONG" in out:
        return "wrong result"
    if "ThreadSanitizer" in out:
        return "thread sanitizer report"
    if "AddressSanitizer" in out:
        return "address sanitizer report"
    if "microtaskparts:" in out:
        return "stack check"
    return "crash (status %d)" % r.returncode


def check(source, label):
    """Builds and runs one version of the header. Returns the first failure."""
    with tempfile.TemporaryDirectory() as d:
        for f in ("context.hpp", "stress.cpp"):
            with open(os.path.join(HERE, f)) as src, open(os.path.join(d, f), "w") as dst:
                dst.write(src.read())
        with open(os.path.join(d, "microtaskparts.hpp"), "w") as f:
            f.write(source)
        exes = {}
        for b in BUILDS:
            exes[b] = build(d, b)
            if exes[b] is None:
                return "does not compile (%s)" % b
        for b, workers, rounds in RUNS:
            failure = run(exes[b], workers, rounds)
            if failure is not None:
                return "%s, in the %s build on %d workers" % (failure, b, workers)
    return None


def main():
    wanted = sys.argv[1:]
    with open(os.path.join(HERE, "microtaskparts.hpp")) as f:
        original = f.read()
    failure = check(original, "original")
    if failure is not None:
        print("the unmutated code fails: " + failure)
        return 1
    print("%-22s passes every run" % "(unmutated)")
    survived = 0
    for name, what, old, new in MUTANTS:
        if wanted and name not in wanted:
            continue
        if original.count(old) != 1:
            print("%-22s NOT APPLIED: its text occurs %d times" % (name, original.count(old)))
            survived += 1
            continue
        failure = check(original.replace(old, new), name)
        if failure is None and name in HARMLESS:
            print("%-22s survived, as expected: %s" % (name, HARMLESS[name]))
        elif failure is None:
            print("%-22s SURVIVED: %s" % (name, what))
            survived += 1
        else:
            print("%-22s killed: %s" % (name, failure))
        sys.stdout.flush()
    return 1 if survived else 0


if __name__ == "__main__":
    sys.exit(main())
