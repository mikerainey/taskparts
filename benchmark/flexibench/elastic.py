#!/usr/bin/env python

# ===========================================
# Elastic task scheduling benchmarking script
# ===========================================

# Imports
# =======

import sys, time, shutil, glob, argparse, psutil, pathlib, fnmatch
sys.setrecursionlimit(150000)

from flexibench import table as T, benchmark as B, query as Q
from taskparts import *

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

default_resource_binding = 'by_core'
default_num_workers = sys_num_workers

experiment_key = 'experiment'
experiments = [ 'high_parallelism' ]

modes = ['dry', 'from_scratch' ] # LATER: add append mode

path_to_benchmarks = '../parlay/'
path_to_binaries = path_to_benchmarks + 'bin/'
path_to_infiles = os.getcwd() + '/../../../infiles'

benchmarks = [ 'bigintadd', 'quickhull' ]

# Benchmark keys
# ==============

binary_key = 'program'
binary_values = binary_extensions

workstealing_key = 'workstealing'
workstealing_values = [ 'elastic', 'multiprogrammed', 'cilk', 'abp',
                        'ywra' #, 'cl'
                       ]

elastic_key = 'elastic'
elastic_default = 's3'
elastic_values = [ elastic_default, 'surplus' ]

semaphore_key = 'semaphore'
semaphore_values = [ 'spin' ]

benchmark_key = 'benchmark'
benchmark_values = benchmarks

prog_keys = [ binary_key, workstealing_key, elastic_key, semaphore_key,
              benchmark_key ]

scheduler_key = 'scheduler'
scheduler_values = [ 'nonelastic', 'multiprogrammed', elastic_default,
                     'elastic', 'elastic2_spin', 'elastic_spin' ]

alpha_key = 'TASKPARTS_ELASTIC_ALPHA'
alpha_values = [2]

beta_key = 'TASKPARTS_ELASTIC_BETA'
beta_values = [128]

infiles_path_key = 'TASKPARTS_BENCHMARK_INFILE_PATH'

# Key types
# ---------

# keys whose associated values are to be passed as environment
# variables
env_arg_keys = taskparts_env_vars + [ alpha_key, beta_key, infiles_path_key ]
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

def mk_parameters(key, vals):
    return T.mk_value([{key: val} for val in vals])

mk_core_bindings = T.mk_cross2(
    T.mk_table1(taskparts_pin_worker_threads_key, 1),
    T.mk_table1(taskparts_resource_binding_key, default_resource_binding))

mk_taskparts_basis = T.mk_cross([
    T.mk_table1(binary_key, 'sta'),
    T.mk_table1(taskparts_numa_alloc_interleaved_key, 1),
    mk_core_bindings,
    T.mk_table1(infiles_path_key, path_to_infiles),])

mk_elastic_shared = T.mk_cross([
    T.mk_table1(workstealing_key, 'elastic'),
    mk_parameters(alpha_key, alpha_values),
    mk_parameters(beta_key, beta_values) ])

# Scheduler configurations
# ------------------------

elastic_scheduler_value = 'elastic' 

# - default scheduler (nonelastic)
mk_sched_nonelastic = T.mk_cross(
    [ T.mk_table1(scheduler_key, 'nonelastic') ])
# - elastic two-level tree scheduler
mk_sched_elastic2 = T.mk_cross(
    [ T.mk_table1(scheduler_key, elastic_scheduler_value),
      mk_elastic_shared,      
      T.mk_table1(elastic_key, elastic_default) ])
# - versions of the two schedulers above, in which the semaphore is
# replaced by our own version that blocks by spinning instead of via
# the OS/semaphore mechanism
mk_sched_elastic2_spin = T.mk_cross(
    [ T.mk_table1(scheduler_key, 'elastic2_spin'),
      mk_elastic_shared,
      T.mk_table1(elastic_key, elastic_default),
      T.mk_table1(semaphore_key, 'spin') ])

# all schedulers
mk_schedulers = T.mk_append(
    [ mk_sched_nonelastic,
      mk_sched_elastic2 ])

# High-parallelism experiment
# ---------------------------

mk_high_parallelism = T.mk_cross(
    [ T.mk_table1(experiment_key, 'high_parallelism'),
      mk_taskparts_basis,
      mk_parameters(benchmark_key, benchmarks),
      mk_schedulers,
      T.mk_table1(taskparts_num_workers_key, default_num_workers) ])

# Benmchmark runs
# ===============

#  given a row, specifies the path of the program to be run
def program_of_row(row):
    assert(benchmark_key in row)
    p = row[benchmark_key]
    if scheduler_key in row:
        s = row[scheduler_key]
        if s == elastic_scheduler_value:
            assert(elastic_key in row)
            p = '.'.join([p, row[elastic_key], elastic_scheduler_value])
    return path_to_binaries + '.'.join([p, 'sta'])

def virtual_run_benchmarks_of_rows(rows):
    i = 1
    n = len(rows)
    for row in rows:
        br_i = B.run_of_row(row, i, n,
                            program_of_row = program_of_row,
                            is_command_line_arg_key = is_command_line_arg_key,
                            is_env_arg_key = is_env_arg_key)
        print(B.string_of_benchmark_run(br_i))
        i += 1

    
rows = T.rows_of(mk_high_parallelism)
print(Q.json_dumps(rows))

virtual_run_benchmarks_of_rows(rows)

def row_of_benchmark_results(br):
    return T.evaluate(T.mk_cross2(T.mk_value([br['row']]), T.mk_value(br['stats'])))

brs = []
i = 1
n = len(rows)
for row in rows:
    br_i = B.run_of_row(row, i, n,
                        program_of_row = program_of_row,
                        is_command_line_arg_key = is_command_line_arg_key,
                        is_env_arg_key = is_env_arg_key)
    i += 1
    br_o = run_taskparts_benchmark(br_i)
    br_o['row'] = row
    brs += [br_o]

print(Q.json_dumps(brs))

print(Q.json_dumps(T.rows_of(T.mk_append([row_of_benchmark_results(br) for br in brs]))))
