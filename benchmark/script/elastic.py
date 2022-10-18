#!/usr/bin/env python

from taskparts_benchmark_run import *
from parameter import *
import glob
import argparse

# Usage:
# ./elastic.py -num_workers <P>
# where P is the number of worker threads you want the benchmarks to use

# Parameters
# ==========

# Taskparts
# ---------

path_to_benchmarks = '../parlay/'

path_to_binaries = path_to_benchmarks + 'bin/'

parser = argparse.ArgumentParser()
parser.add_argument('-num_workers', type=str, required=True)
args = parser.parse_args()
num_workers = args.num_workers
print(num_workers)

outfile = 'results.json'

num_benchmark_repeat = 2

# Elastic scheduling
# ------------------

benchmark_skips = [ 'cyclecount' ]

benchmarks = [ x for x
               in [os.path.basename(x).split('.')[0] for x
                   in glob.glob(path_to_benchmarks + "*.cpp")]
               if x not in benchmark_skips ]

# Benchmark keys
# ==============

binary_key = 'binary'
binary_values = [ 'opt', 'sta', 'log', 'dbg' ]

scheduler_key = 'scheduler'
scheduler_values = [ 'elastic', 'multiprogrammed' ]

elastic_key = 'elastic'
elastic_values = [ 'surplus', 'tree' ]

semaphore_key = 'semaphore'
semaphore_values = [ 'spin' ]

benchmark_key = 'benchmark'
benchmark_values = [ 'cyclecount', 'fft' ]

prog_keys = [ binary_key, scheduler_key, elastic_key,
              semaphore_key, benchmark_key ]

override_granularity_key = 'override_granularity'

# Impact of low parallelism experiment
# ------------------------------------

# Impact of phased workload experiment
# ------------------------------------

force_sequential_key = 'force_sequential'

k_key = 'k'

m_key = 'm'

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

# Conversion from row to run
# --------------------------

env_arg_keys = taskparts_env_vars

def flatten(l):
    return [item for sublist in l for item in sublist]

def run_of_row(row):
    p = prog_of_row(row)
    cs = flatten([ ['-' + kv['key'], str(kv['val'])]
                   for kv in row
                   if (kv['key'] not in prog_keys) and
                   (kv['key'] not in env_arg_keys) ])
    es = [ {"var": kv['key'], "val": str(kv['val'])}
           for kv in row
           if (kv['key'] not in prog_keys) and
           (kv['key'] in env_arg_keys) ]
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

mk_scheds = mk_append_sequence(
    [ mk_parameter('binary', 'sta'),
      mk_cross_sequence([ mk_parameter('binary', 'sta'),
                          mk_parameter('scheduler', 'elastic'),
                          mk_parameter('elastic', 'surplus') ]) ])

r1 = mk_cross_sequence(
    [ mk_parameters('benchmark', [ 'fft', 'bigintadd' ]),
      mk_scheds,
      mk_parameter(taskparts_num_workers_key, num_workers) ])

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

#pretty_print_json(run_elastic_benchmarks(r1))

def json_dumps(thing):
    return json.dumps(
        thing,
        ensure_ascii=False,
        sort_keys=True,
        indent=2,
        separators=(',', ':'),
    )

def write_string_to_file_path(bstr, file_path = './results.json', verbose = True):
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

write_benchmark_to_file_path(run_elastic_benchmarks(r1), file_path = outfile)
