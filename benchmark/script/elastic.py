#!/usr/bin/env python

from taskparts_benchmark_run import *
from parameter import *
import glob, argparse, psutil, pathlib

# ===========================================
# Elastic task scheduling benchmarking script
# ===========================================

# TODOs:
#   - experiment with varying:
#      - TASKPARTS_NB_TO_WAKE_ON_SURPLUS_INCREASE
#      - TASKPARTS_NB_STEAL_ATTEMPTS
#      - TASKPARTS_ELASTIC_TREE_VICTIM_SELECTION_BY_TREE
#      - inject a little randomness to the process of going to sleep,
#        in the hopes of detecting some contention on the wake->sleep
#        and sleep->wake transitions
#      - use compare_exchange_with_backoff
#      - try stressing the schedulers w/ the challenge workload in mixed.hpp
#   - incremental snapshots of experiments and output of results to git
#   - try the version of scalable elastic that uses the tree to sample for victim workers
#   - save benchmark-run history in addition to the output table
#   - make it possible to configure warmup secs
#   - print to stdout exectime preview and counter value of each benchmark
#   - much later: experiment with idea of a parameter expression that expresses
#     a key waiting for a value

# Parameters
# ==========

sys_num_workers = psutil.cpu_count(logical=False)

# Command line
# ------------

parser = argparse.ArgumentParser('Benchmark elastic task scheduling')
parser.add_argument('-num_workers', type=int, required=False,
                    default = sys_num_workers,
                    help = 'number of worker threads to use in benchmarks')
parser.add_argument('--virtual_run', dest ='virtual_run',
                    action ='store_true',
                    help ='only print the benchmarks to be run')
parser.add_argument('-num_benchmark_repeat', type=int, required=False,
                    default = 1,
                    help = 'number of times to repeat each benchmark run (default 1)')
parser.add_argument('-results_outfile', type=str, required=False,
                    default = 'results.json',
                    help = 'path to the file in which to write benchmark results (default results.json)')
parser.add_argument('--verbose', dest ='verbose',
                    action ='store_true',
                    help ='verbose mode')
args = parser.parse_args()

# Taskparts
# ---------

path_to_benchmarks = '../parlay/'

path_to_binaries = path_to_benchmarks + 'bin/'

# Elastic scheduling
# ------------------

# list of benchmarks to skip
benchmark_skips = [ 'cyclecount' ]

# list of benchmarks to run
benchmarks = [ x for x
               in [os.path.basename(x).split('.')[0] for x
                   in glob.glob(path_to_benchmarks + "*.cpp")]
               if x not in benchmark_skips ]

# uncomment to override the list of benchmarks above
benchmarks = [ 'fft', 'quickhull',
               'bellmanford', 'knn',
               'samplesort', 'suffixarray', 'karatsuba', 'setcover',
               'filterkruskal', 'bigintadd', 
               'betweennesscentrality', 'bucketeddijkstra',
               'cartesiantree', 'graphcolor',
               # something seems off...
               'kcore' ]

benchmarks = [ 'fft', 'quickhull' ]

# Benchmark keys
# ==============

binary_key = 'binary'
binary_values = [ 'opt', 'sta', 'log', 'dbg' ]

chaselev_key = 'chaselev'
chaselev_values = [ 'elastic', 'multiprogrammed' ]

elastic_key = 'elastic'
elastic_values = [ 'surplus2', 'surplus' ]

semaphore_key = 'semaphore'
semaphore_values = [ 'spin' ]

benchmark_key = 'benchmark'
benchmark_values = benchmarks

prog_keys = [ binary_key, chaselev_key, elastic_key, semaphore_key,
              benchmark_key ]

scheduler_key = 'scheduler'
scheduler_values = [ 'nonelastic', 'multiprogrammed',
                     'elastic2', 'elastic',
                     'elastic2_spin', 'elastic_spin' ]

experiment_key = 'experiment'
experiment_values = [ 'high-parallelism', 'low-parallelism',
                      'parallel-serial-mix' ]

# Key types
# ---------

# keys whose associated values are to be passed as environment
# variables
env_arg_keys = taskparts_env_vars
# keys that are not meant to be passed at all (just for annotating
# rows)
silent_keys = [ scheduler_key, experiment_key ]

def is_prog_key(k):
    return (k in prog_keys)
def is_silent_key(k):
    return (k in silent_keys)
def is_env_arg_key(k):
    return (k in env_arg_keys) and not(is_prog_key(k))
def is_command_line_arg_key(k):
    return not(is_silent_key(k)) and not(is_env_arg_key(k)) and not(is_prog_key(k))

# Benchmarking configuration
# ==========================

mk_core_bindings = mk_cross(
    mk_parameter(taskparts_pin_worker_threads_key, 1),
    mk_parameter(taskparts_resource_binding_key,
                 taskparts_resource_binding_by_core))

mk_taskparts_basis = mk_cross_sequence([
    mk_parameter(binary_key, 'sta'),
    mk_parameter(taskparts_numa_alloc_interleaved_key, 1),
    mk_core_bindings ])

# High-parallelism experiment
# ---------------------------

# - default scheduler (chaselev)
mk_sched_chaselev = mk_cross_sequence(
    [ mk_parameter(scheduler_key, 'nonelastic') ])
# - elastic two-level tree scheduler
mk_sched_elastic2 = mk_cross_sequence(
    [ mk_parameter(scheduler_key, 'elastic2'),
      mk_parameter(chaselev_key, 'elastic'),
      mk_parameter(elastic_key, 'surplus2') ])
# - elastic multi-level tree scheduler
mk_sched_elastic = mk_cross_sequence(
    [ mk_parameter(scheduler_key, 'elastic'),
      mk_parameter(chaselev_key, 'elastic'),
      mk_parameter(elastic_key, 'surplus') ] )
# - versions of the two schedulers above, in which the semaphore is
# replaced by our own version that blocks by spinning instead of via
# the OS/semaphore mechanism
mk_sched_elastic2_spin = mk_cross_sequence(
    [ mk_parameter(scheduler_key, 'elastic2_spin'),
      mk_parameter(chaselev_key, 'elastic'),
      mk_parameter(elastic_key, 'surplus2'),
      mk_parameter(semaphore_key, 'spin') ])
mk_sched_elastic_spin = mk_cross_sequence(
    [ mk_parameter(scheduler_key, 'elastic_spin'),
      mk_parameter(chaselev_key, 'elastic'),
      mk_parameter(elastic_key, 'surplus'),
      mk_parameter(semaphore_key, 'spin')  ] )

# all schedulers
mk_scheds = mk_append_sequence(
    [ mk_sched_chaselev,
      mk_sched_elastic2, mk_sched_elastic,
      mk_sched_elastic2_spin, mk_sched_elastic_spin ])

mk_high_parallelism = mk_cross_sequence(
    [ mk_parameter(experiment_key, 'high-parallelism'),
      mk_taskparts_basis,
      mk_parameters(benchmark_key, benchmarks),
      mk_scheds,
      mk_parameter(taskparts_num_workers_key, args.num_workers) ])

# Low-parallelism experiment
# --------------------------

override_granularity_key = 'override_granularity'

mk_low_parallelism = mk_cross_sequence(
    [ mk_parameter(experiment_key, 'low-parallelism'),
      mk_taskparts_basis,
      mk_parameters(benchmark_key, benchmarks),
      mk_scheds,
      mk_parameter(taskparts_num_workers_key, args.num_workers),
      mk_parameter(override_granularity_key, 1) ])

# Parallel-sequential-mix experiment
# ----------------------------------

mk_parallel_sequential_mix = mk_cross_sequence(
    [ mk_parameter(experiment_key, 'parallel-sequential-mix'),
      mk_taskparts_basis,
      mk_parameters(benchmark_key, benchmarks),
      mk_scheds,
      mk_parameter(taskparts_num_workers_key, args.num_workers),
      mk_parameter('force_sequential', 1),
      mk_parameter('k', 3),
      mk_parameter('m', 2) ])

# Multiprogrammed work-stealing experiment
# ----------------------------------------

mk_sched_multiprogrammed = mk_cross_sequence(
    [ mk_parameter(scheduler_key, 'multiprogrammed'),
      mk_parameter(chaselev_key, 'multiprogrammed') ])

mk_multiprogrammed = mk_cross_sequence(
    [ mk_parameter(experiment_key, 'multiprogrammed'),
      mk_taskparts_basis,
      mk_parameters(benchmark_key, benchmarks),
      mk_append_sequence([
          mk_sched_chaselev,
          mk_sched_elastic2,
          mk_sched_multiprogrammed ]),
      mk_parameters(taskparts_num_workers_key,
                    [ args.num_workers * i for i in [1, 2, 3, 4] ]) ])

# Cilk experiment
# ---------------

# Benchmark runs
# ==============

# Programs
# --------

def prog_of_row(row):
    prog = ''
    for p in prog_keys:
        vo = val_of_key_in_row(row, p)
        if vo == []:
            continue
        v = vo[0]
        if p != prog_keys[0]:
            v = v + '.'
        prog = v + prog
    return prog

def maybe_build_benchmark(b):
    path = pathlib.Path(path_to_binaries + b)
    if path.is_file():
        return
    print('$', end = ' ')
    cmd = 'make ' + b
    print(cmd)
    if args.virtual_run:
        return
    os_env = os.environ.copy()
    env_args = { }
    env = {**os_env, **env_args}
    current_child = subprocess.Popen(cmd, shell = True, # env = env,
                                     stdout = subprocess.PIPE, stderr = subprocess.PIPE,
                                     cwd = path_to_benchmarks)
    child_stdout, child_stderr = current_child.communicate(timeout = 1000)
    child_stdout = child_stdout.decode('utf-8')
    child_stderr = child_stderr.decode('utf-8')
    return_code = current_child.returncode
    if return_code != 0:
        print('Error: benchmark build returned error code ' + str(return_code))
        print('stdout:')
        print(str(child_stdout))
        print('stderr:')
        print(str(child_stderr))
        exit

# Conversion from row to run
# --------------------------

def flatten(l):
    return [item for sublist in l for item in sublist]

def run_of_row(row, i, n):
    p = prog_of_row(row)
    print('[' + str(i) + '/' + str(n) + ']')
    maybe_build_benchmark(p)
    print('$', end = ' ')
    cs = flatten([ ['-' + kv['key'], str(kv['val'])]
                   for kv in row
                   if is_command_line_arg_key(kv['key']) ])
    es = [ {"var": kv['key'], "val": str(kv['val'])}
           for kv in row
           if is_env_arg_key(kv['key']) ]
    r = { "benchmark_run":
          { "path_to_executable": p,
            "cl_args": cs,
            "env_args": es } }
    validate_benchmark_run(r)
    return r

# br_i = run_of_row([ {"key": "benchmark", "val": "fft"},
#                     {"key": "binary", "val": "sta"},
#                     {"key": "elastic", "val": "surplus"},
#                     {"key": "n1", "val": 123},
#                     {"key": "m", "val": 456},
#                     {"key": "m2", "val": 4567},
#                     {"key": "scheduler", "val": "elastic"},
#                     {"key": taskparts_num_workers_key, "val": args.num_workers}
#                    ], 0, 1)
# pretty_print_json(br_i)
# #br_i = read_benchmark_from_file_path('taskparts_benchmark_run_example1.json')
# r = run_taskparts_benchmark(br_i,num_repeat=args.num_benchmark_repeat,verbose=True,cwd=path_to_binaries)
# pretty_print_json(r)

# Run a number of benchmarks given as rows
# ----------------------------------------

def run_benchmarks_of_rows(rows):
    brs = []
    i = 1
    n = len(rows)
    for row in rows:
        br_i = run_of_row(row, i, n)
        br_o = run_taskparts_benchmark(br_i,
                                       num_repeat=args.num_benchmark_repeat,
                                       verbose=True,
                                       cwd=path_to_binaries)
        br_o['row'] = row
        brs += [br_o]
        i += 1
    return brs

def virtual_run_benchmarks_of_rows(rows):
    i = 1
    n = len(rows)
    for row in rows:
        br_i = run_of_row(row, i, n)
        print(string_of_benchmark_run(br_i))
        i += 1

# r = run_benchmarks_of_rows([
#     [ {"key": "benchmark", "val": "fft"},
#       {"key": "binary", "val": "sta"},
#       {"key": "elastic", "val": "surplus"},
#       {"key": "n1", "val": 123},
#       {"key": "m", "val": 456},
#       {"key": "m2", "val": 4567},
#       {"key": "scheduler", "val": "elastic"},
#       {"key": taskparts_num_workers_key, "val": args.num_workers}
#      ] ])
# pretty_print_json(r)

# Run a batch of benchmarks given by a parameter expression
# ---------------------------------------------------------

def row_of_benchmark_results(br):
    stats = {'value': [ dictionary_to_row(row) for row in br['stats'] ] }
    e = {'value': [ br['row'] ] }
    return mk_cross(e, stats)

def run_elastic_benchmarks(e):
    rows = rows_of(eval(e))
    brs = run_benchmarks_of_rows(rows)
    results = mk_unit()
    for br in brs:
        results = mk_append(row_of_benchmark_results(br), results)
    return eval(results)

def virtual_run_elastic_benchmarks(e):
    rows = rows_of(eval(e))
    virtual_run_benchmarks_of_rows(rows)

# Results-file output
# ===================

def json_dumps(thing):
    return json.dumps(
        thing,
        ensure_ascii=False,
        sort_keys=True,
        indent=2,
        separators=(',', ':'),
    )

def write_string_to_file_path(bstr, file_path = args.results_outfile, verbose = True):
    with open(file_path, 'w', encoding='utf-8') as fd:
        fd.write(bstr)
        fd.close()
        if verbose:
            print('Wrote benchmark to file: ' + file_path)

def write_benchmark_to_file_path(benchmark, file_path = '', verbose = True):
    bstr = json_dumps(benchmark)
    file_path = file_path if file_path != '' else 'results-' + get_hash(bstr) + '.json'
    write_string_to_file_path(bstr, file_path, verbose)
    return file_path

# Driver
# ======

if not(args.virtual_run):
    write_benchmark_to_file_path(run_elastic_benchmarks(mk_high_parallelism), file_path = args.results_outfile)
else:
    virtual_run_elastic_benchmarks(mk_high_parallelism)
    virtual_run_elastic_benchmarks(mk_low_parallelism)
    virtual_run_elastic_benchmarks(mk_parallel_sequential_mix)
    virtual_run_elastic_benchmarks(mk_multiprogrammed)
