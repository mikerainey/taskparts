#!/usr/bin/env python

from taskparts_benchmark_run import *
from parameter import *
import glob, argparse, psutil, pathlib

# ===========================================
# Elastic task scheduling benchmarking script
# ===========================================

# TODOs:
#   - incremental snapshots of experiments
#   - save benchmark-run history in addition to the output table

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

num_workers = args.num_workers

results_outfile = args.results_outfile

num_benchmark_repeat = args.num_benchmark_repeat

virtual_run = args.virtual_run

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
benchmarks = [ 'fft', 'bigintadd' ]

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

# Impact of low parallelism experiment
# ------------------------------------

override_granularity_key = 'override_granularity'

# Impact of phased workload experiment
# ------------------------------------

force_sequential_key = 'force_sequential'

k_key = 'k'

m_key = 'm'

# Key types
# ---------

# keys whose associated values are to be passed as environment
# variables
env_arg_keys = taskparts_env_vars
# keys that are not meant to be passed at all (just for annotating
# rows)
silent_keys = [ scheduler_key ]

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

# default scheduler (chaselev)
mk_sched_chaselev = mk_cross_sequence(
    [ mk_parameter(scheduler_key, 'nonelastic'),
      mk_parameter(binary_key, 'sta') ])
# elastic two-level tree scheduler
mk_sched_elastic2 = mk_cross_sequence(
    [ mk_parameter(scheduler_key, 'elastic2'),
      mk_parameter(binary_key, 'sta'),
      mk_parameter(chaselev_key, 'elastic'),
      mk_parameter(elastic_key, 'surplus2') ])
# elastic multi-level tree scheduler
mk_sched_elastic = mk_cross_sequence(
    [ mk_parameter(scheduler_key, 'elastic'),
      mk_parameter(binary_key, 'sta'),
      mk_parameter(chaselev_key, 'elastic'),
      mk_parameter(elastic_key, 'surplus') ] )
# versions of the two schedulers above, in which the semaphore is
# replaced by our own version that blocks by spinning instead of via
# the OS/semaphore mechanism
mk_sched_elastic2_spin = mk_cross_sequence(
    [ mk_parameter(scheduler_key, 'elastic2_spin'),
      mk_parameter(binary_key, 'sta'),
      mk_parameter(chaselev_key, 'elastic'),
      mk_parameter(elastic_key, 'surplus2'),
      mk_parameter(semaphore_key, 'spin') ])
mk_sched_elastic_spin = mk_cross_sequence(
    [ mk_parameter(scheduler_key, 'elastic_spin'),
      mk_parameter(binary_key, 'sta'),
      mk_parameter(chaselev_key, 'elastic'),
      mk_parameter(elastic_key, 'surplus'),
      mk_parameter(semaphore_key, 'spin')  ] )

# all schedulers
mk_scheds = mk_append_sequence(
    [ mk_sched_chaselev,
      mk_sched_elastic2, mk_sched_elastic,
      mk_sched_elastic2_spin, mk_sched_elastic_spin ])
# high-parallelism experiment
mk_high_parallelism = mk_cross_sequence(
    [ mk_parameters(benchmark_key, benchmarks),
      mk_scheds,
      mk_parameter(taskparts_num_workers_key, num_workers) ])

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
    cmd = 'make ' + b
    print(cmd)
    if virtual_run:
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

def run_of_row(row):
    p = prog_of_row(row)
    maybe_build_benchmark(p)
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
#                     {"key": taskparts_num_workers_key, "val": num_workers}
#                    ])
# pretty_print_json(br_i)
# #br_i = read_benchmark_from_file_path('taskparts_benchmark_run_example1.json')
# r = run_taskparts_benchmark(br_i,num_repeat=num_benchmark_repeat,verbose=True,cwd=path_to_binaries)
# pretty_print_json(r)

# Run a number of benchmarks given as rows
# ----------------------------------------

def run_benchmarks_of_rows(rows):
    brs = []
    for row in rows:
        br_i = run_of_row(row)
        br_o = run_taskparts_benchmark(br_i,
                                       num_repeat=num_benchmark_repeat,
                                       verbose=True,
                                       cwd=path_to_binaries)
        br_o['row'] = row
        brs += [br_o]
    return brs

def virtual_run_benchmarks_of_rows(rows):
    for row in rows:
        br_i = run_of_row(row)
        print(string_of_benchmark_run(br_i))

# r = run_benchmarks_of_rows([
#     [ {"key": "benchmark", "val": "fft"},
#       {"key": "binary", "val": "sta"},
#       {"key": "elastic", "val": "surplus"},
#       {"key": "n1", "val": 123},
#       {"key": "m", "val": 456},
#       {"key": "m2", "val": 4567},
#       {"key": "scheduler", "val": "elastic"},
#       {"key": taskparts_num_workers_key, "val": num_workers}
#      ] ])
# pretty_print_json(r)

# Run a batch of benchmarks given by a parameter expression
# ---------------------------------------------------------

#r = eval(r1)
#pretty_print_json(run_benchmarks_of_rows(rows_of(r)))

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

#pretty_print_json(run_elastic_benchmarks(r1))

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

def write_string_to_file_path(bstr, file_path = results_outfile, verbose = True):
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

if not(virtual_run):
    write_benchmark_to_file_path(run_elastic_benchmarks(mk_high_parallelism), file_path = results_outfile)
else:
    virtual_run_elastic_benchmarks(mk_high_parallelism)
