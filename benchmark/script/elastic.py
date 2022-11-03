#!/usr/bin/env python

import sys, time, shutil
sys.setrecursionlimit(150000)
from taskparts_benchmark_run import *
from parameter import *
import glob, argparse, psutil, pathlib

# ===========================================
# Elastic task scheduling benchmarking script
# ===========================================

# TODOs:
#   - experiment with varying:
#      - try all different pinning options
#      - TASKPARTS_ELASTIC_ALPHA
#      - TASKPARTS_NB_STEAL_ATTEMPTS
#      - TASKPARTS_ELASTIC_TREE_VICTIM_SELECTION_BY_TREE
#      - inject a little randomness to the process of going to sleep,
#        in the hopes of detecting some contention on the wake->sleep
#        and sleep->wake transitions
#      - use compare_exchange_with_backoff
#      - try stressing the schedulers w/ the challenge workload in mixed.hpp
#   - Cilk experiment (?)
#   - incremental snapshots of experiments and output of results to git
#   - try the version of scalable elastic that uses the tree to sample for victim workers
#   - save benchmark-run history in addition to the output table
#   - make it possible to configure warmup secs
#   - print to stdout exectime preview and counter value of each benchmark
#   - much later: experiment with idea of a parameter expression that expresses
#     a key waiting for a value

# Parameters
# ==========

timestr = time.strftime("%Y-%m-%d-%H-%M-%S")

default_local_results_path = 'results-' + timestr

taskparts_home = '../../'

default_remote_results_path = taskparts_home + '../draft-elastic/drafts/draft-elastic/experiments/'

# default setting for the nb of worker threads to be used by taskparts
# (can be overridden by -num-workers); should be the count of the
# number of cores in the calling system
sys_num_workers = psutil.cpu_count(logical=False)

experiment_key = 'experiment'
experiments = [ 'high_parallelism', 'low_parallelism',
                'parallel_sequential_mix', 'multiprogrammed' ]

modes = ['dry', 'from_scratch' ] # LATER: add append mode

path_to_benchmarks = '../parlay/'
path_to_binaries = path_to_benchmarks + 'bin/'

# list of all benchmarks in the parlay folder
#############################################################
# benchmarks = [os.path.basename(x).split('.')[0] for x     #
#               in glob.glob(path_to_benchmarks + "*.cpp")] #
#############################################################

# uncomment to override the list of benchmarks above
all_benchmarks = [ 'quickhull',
                   'bellmanford', 'knn',
                   'samplesort', 'suffixarray', 'setcover',
                   'filterkruskal', 'bigintadd', 
                   'betweennesscentrality', 'bucketeddijkstra',
                   'cartesiantree', 'graphcolor' ]
# something seems off with this one...
#               'kcore'
# 'fft' not compiling
# 'karatsuba' segfaulting randomly

few_benchmarks = [ 'bigintadd', 'quickhull' ]

# Command line arguments
# ----------------------

parser = argparse.ArgumentParser('Benchmark elastic task scheduling')

parser.add_argument('-num_workers', type=int, required=False,
                    default = sys_num_workers,
                    help = 'number of worker threads to use in benchmarks')
parser.add_argument('-mode', choices = modes, default = 'dry',
                    help ='operating modes of this script')
parser.add_argument('-experiment', action='append',
                    help = ('any one of ' + str(experiments)))
parser.add_argument('--few_benchmarks', dest ='few_benchmarks',
                    action ='store_true',
                    help = ('run only benchmarks ' + str(few_benchmarks)))
parser.add_argument('-only_benchmark', action='append',
                    help = 'specifies a benchmark to run')
parser.add_argument('-skip_benchmark', action='append',
                    help = 'specifies a benchmark to not run')
parser.add_argument('-num_benchmark_repeat', type=int, required=False,
                    default = 1,
                    help = 'number of times to repeat each benchmark run (default 1)')
parser.add_argument('--verbose', dest ='verbose',
                    action ='store_true',
                    help ='verbose mode')
parser.add_argument('-binding', choices = taskparts_resource_bindings, default = 'by_core',
                    help ='worker-pthread-to-resource binding policy')
parser.add_argument('-local_results_path', # action='append',
                    help = 'path to a folder in which to generate results files; default: ' +
                    default_local_results_path)
parser.add_argument('-remote_results_path', #action='append',
                    help = 'path to a folder in the git repo in which to commit results files; default: ' +
                    default_remote_results_path)
parser.add_argument('--export_results', dest ='export_results',
                    action ='store_true',
                    help ='select to commit the results files to the git repository located in the results path')

args = parser.parse_args()

local_results_path = default_local_results_path if args.local_results_path == None else args.local_results_path

remote_results_path = default_remote_results_path if args.remote_results_path == None else args.remote_results_path

export_results = False if args.export_results == None else args.export_results

experiment_values = experiments if args.experiment == None else args.experiment
for e in experiment_values:
    if e not in experiments:
        raise SystemExit('Experiment ' + e + ' not in ' + str(experiments))

is_dry_run = args.mode == 'dry'

benchmarks = all_benchmarks if not(args.few_benchmarks) else few_benchmarks

only_benchmarks = args.only_benchmark
if only_benchmarks != None:
    for b in only_benchmarks:
        if b not in all_benchmarks:
            raise SystemExit('Benchmark ' + b + ' not in ' + str(all_benchmarks))
    benchmarks = only_benchmarks

skip_benchmarks = args.skip_benchmark
if skip_benchmarks != None:
    for b in skip_benchmarks:
        if b not in all_benchmarks:
            raise SystemExit('Benchmark ' + b + ' not in ' + str(all_benchmarks))
    benchmarks = [ b for b in benchmarks if b not in skip_benchmarks ]


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
    mk_parameter(taskparts_resource_binding_key, args.binding))

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
      mk_sched_elastic2,
      mk_sched_elastic2_spin ])

mk_high_parallelism = mk_cross_sequence(
    [ mk_parameter(experiment_key, 'high_parallelism'),
      mk_taskparts_basis,
      mk_parameters(benchmark_key, benchmarks),
      mk_scheds,
      mk_parameter(taskparts_num_workers_key, args.num_workers) ])

# Low-parallelism experiment
# --------------------------

override_granularity_key = 'override_granularity'

mk_low_parallelism = mk_cross_sequence(
    [ mk_parameter(experiment_key, 'low_parallelism'),
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

# TODO

# All experiments
# ---------------

all_experiments = { 'high_parallelism': mk_high_parallelism,
                    'low_parallelism': mk_low_parallelism,
                    'parallel_sequential_mix': mk_parallel_sequential_mix,
                    'multiprogrammed': mk_multiprogrammed }
experiments_to_run = { k: all_experiments[k] for k in experiment_values }

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
    if is_dry_run:
        return
    current_child = subprocess.Popen(cmd, shell = True,
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

# row is the i^th of n rows in total
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
    return eval(results), brs

def virtual_run_elastic_benchmarks(e):
    rows = rows_of(eval(e))
    virtual_run_benchmarks_of_rows(rows)

# Results-file output
# ===================

timestr = time.strftime("%Y-%m-%d-%H-%M-%S")

def json_dumps(thing):
    return json.dumps(
        thing,
        ensure_ascii=False,
        sort_keys=True,
        indent=2,
        separators=(',', ':'),
    )

def write_string_to_file_path(bstr, file_path, verbose = True):
    with open(file_path, 'w', encoding='utf-8') as fd:
        fd.write(bstr)
        fd.close()
        if verbose:
            print('Wrote to file: ' + file_path)

def write_json_to_file_path(j, file_path = '', verbose = True):
    bstr = json_dumps(j)
    timestr = time.strftime("%Y-%m-%d-%H-%M-%S")
    file_path = file_path if file_path != '' else 'results-' + timestr + get_hash(bstr) + '.json'
    write_string_to_file_path(bstr, file_path, verbose)
    return file_path

# Driver
# ======

def export_results_to_git():
    if not(export_results):
        return
    if not(os.path.exists(local_results_path)):
        return
    bn = os.path.basename(os.path.abspath(local_results_path))
    rrp = remote_results_path + bn
    print('Exporting local results in ' + local_results_path +
          ' to the remote path ' + rrp)
    shutil.copytree(local_results_path, rrp, dirs_exist_ok=True)
    cmd = 'git add ' + bn
    exp_path, _ = os.path.split(rrp)
    current_child = subprocess.Popen(cmd, shell = True,
                                     stdout = subprocess.PIPE, stderr = subprocess.PIPE,
                                     cwd = exp_path)
    child_stdout, child_stderr = current_child.communicate(timeout = 1000)
    child_stdout = child_stdout.decode('utf-8')
    child_stderr = child_stderr.decode('utf-8')
    return_code = current_child.returncode
    if return_code != 0:
        print('Error: export to git returned error code ' + str(return_code))
        print('stdout:')
        print(str(child_stdout))
        print('stderr:')
        print(str(child_stderr))
        exit


print('=============================================')
if is_dry_run:
    print('Dry run:')
else:
    print('From-scratch run:')
print('\tExperiments: ' + str(experiment_values))
print('\tBenchmarks: ' + str(benchmarks))
print('=============================================\n')

for (e, mk) in experiments_to_run.items():
    pfx = local_results_path + '/' + e
    results_file =  pfx + '-results.json'
    trace_file =  pfx + '-trace.json'
    print('\n------------------------')
    print('Experiment: ' + e)
    print('\tresults to be written to ' + results_file)
    print('\ttrace to be written to ' + trace_file)
    print('------------------------\n')
    if not(is_dry_run) and args.verbose:
        virtual_run_elastic_benchmarks(mk)
    elif is_dry_run:
        virtual_run_elastic_benchmarks(mk)
        continue
    if not(os.path.exists(local_results_path)):
        os.makedirs(local_results_path)
    print('\nStarting to run benchmarks\n')
    results, traces = run_elastic_benchmarks(mk)
    write_json_to_file_path(results, file_path = results_file)
    write_json_to_file_path(traces, file_path = trace_file)
    
export_results_to_git()
