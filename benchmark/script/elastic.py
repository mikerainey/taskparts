#!/usr/bin/env python

import sys, time, shutil
sys.setrecursionlimit(150000)
from taskparts_benchmark_run import *
from parameter import *
import glob, argparse, psutil, pathlib, fnmatch

# ===========================================
# Elastic task scheduling benchmarking script
# ===========================================

# TODOs:
#   - See if adaptive feedback helps or hurts w/ perf of elastic

# LATERs:
#   - try stressing the schedulers w/ the challenge workload in mixed.hpp
#   - On start of from_scratch run, rebuild all benchmarks, unless specified otherwise
#   - incremental snapshots of experiments and output of results to git
#   - try the version of scalable elastic that uses the tree to sample for victim workers TASKPARTS_ELASTIC_TREE_VICTIM_SELECTION_BY_TREE
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
experiments = [ 'high_parallelism',
                'parallel_sequential_mix', 'graph',
                'cilk_shootout',
                'vary_elastic_params',
                'multiprogrammed',
                'try_without_binding'
               ]

modes = ['dry', 'from_scratch' ] # LATER: add append mode

path_to_benchmarks = '../parlay/'
path_to_binaries = path_to_benchmarks + 'bin/'
path_to_infiles = os.getcwd() + '/../../../infiles'

# list of all benchmarks in the parlay folder
#############################################################
# benchmarks = [os.path.basename(x).split('.')[0] for x     #
#               in glob.glob(path_to_benchmarks + "*.cpp")] #
#############################################################

pbbs_benchmarks = [ 'classify', 'raycast', 'dedup' ] # 'index', (index always segfaults currently)

parlay_benchmarks = [ 'quickhull', 'bellmanford', 'samplesort',
                      'suffixarray', 'setcover', 
                      'bigintadd', 'btwcentrality',
                      'trianglecount', 'cartesiantree', 'graphcolor',
                      'nbodyfmm', 'rabinkarp', 'knn', 'huffmantree',
                      'fft', 'knuthmorrispratt', 'kruskal'
                      #'spectralseparator' # really slow: 10s but highly parallel
                      #'kcore',      # seems best w/ high diameter graph;
                      #'filterkruskal',
                      #'bucketeddijkstra' ?? slow?
                     ]

all_benchmarks = pbbs_benchmarks + parlay_benchmarks

broken_benchmarks = [ 
                      'karatsuba', # segfaults randomly
    
                     ]

few_benchmarks = [ 'bigintadd', 'quickhull' ] #, 'samplesort', 'suffixarray' ]

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
parser.add_argument('--use_elastic_spin', dest ='use_elastic_spin',
                    action ='store_true',
                    help = ('run only benchmarks ' + str(few_benchmarks)))
parser.add_argument('-only_benchmark', action='append',
                    help = 'specifies a benchmark to run')
parser.add_argument('-skip_benchmark', action='append',
                    help = 'specifies a benchmark to not run')
parser.add_argument('-num_benchmark_repeat', type=int, required=False,
                    default = 1,
                    help = 'number of times to repeat each benchmark run (default 1)')
parser.add_argument('-alpha', type=int, required=False,
                    default = [2], action='append',
                    help = 'elastic parameter')
parser.add_argument('-beta', type=int, required=False,
                    default = [128], action='append',
                    help = 'elastic parameter')
parser.add_argument('--verbose', dest ='verbose',
                    action ='store_true',
                    help ='verbose mode')
parser.add_argument('-binding', choices = taskparts_resource_bindings, default = 'by_core',
                    help ='worker-pthread-to-resource binding policy')
parser.add_argument('-local_results_path',
                    help = 'path to a folder in which to generate results files; default: ' +
                    default_local_results_path)
parser.add_argument('-remote_results_path',
                    help = 'path to a folder in the git repo in which to commit results files; default: ' +
                    default_remote_results_path)
parser.add_argument('--export_results', dest ='export_results',
                    action ='store_true',
                    help ='select to commit the results files to the git repository located in the results path')
parser.add_argument('-merge_results', action='append',
                    help = 'specifies the path of a results folder to merge with the output of the current run')

parser.add_argument('-load_results_file', action='append',
                    help = 'specifies a results file to load')
parser.add_argument('-split_into_files_by_keys', action='append',
                    help = 'specifies a results key to split into multiple files by all values')

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
alpha_values = args.alpha

beta_key = 'TASKPARTS_ELASTIC_BETA'
beta_values = args.beta

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

mk_core_bindings = mk_cross(
    mk_parameter(taskparts_pin_worker_threads_key, 1),
    mk_parameter(taskparts_resource_binding_key, args.binding))

mk_taskparts_basis = mk_cross_sequence([
    mk_parameter(binary_key, 'sta'),
    mk_parameter(taskparts_numa_alloc_interleaved_key, 1),
    mk_core_bindings,
    mk_parameter(infiles_path_key, path_to_infiles),])

mk_elastic_shared = mk_cross_sequence([
    mk_parameter(workstealing_key, 'elastic'),
    mk_parameters(alpha_key, alpha_values),
    mk_parameters(beta_key, beta_values) ])

# Scheduler configurations
# ------------------------

elastic_scheduler_value = 'elastic2' 

# - default scheduler (nonelastic)
mk_sched_nonelastic = mk_cross_sequence(
    [ mk_parameter(scheduler_key, 'nonelastic') ])
# - elastic two-level tree scheduler
mk_sched_elastic2 = mk_cross_sequence(
    [ mk_parameter(scheduler_key, elastic_scheduler_value),
      mk_elastic_shared,      
      mk_parameter(elastic_key, elastic_default) ])
# - versions of the two schedulers above, in which the semaphore is
# replaced by our own version that blocks by spinning instead of via
# the OS/semaphore mechanism
mk_sched_elastic2_spin = mk_cross_sequence(
    [ mk_parameter(scheduler_key, 'elastic2_spin'),
      mk_elastic_shared,
      mk_parameter(elastic_key, elastic_default),
      mk_parameter(semaphore_key, 'spin') ])

mk_sched_elastic_all = mk_append(mk_sched_elastic2, mk_sched_elastic2_spin if args.use_elastic_spin else mk_unit())

# all schedulers
mk_schedulers = mk_append_sequence(
    [ mk_sched_nonelastic,
      mk_sched_elastic_all ])

# High-parallelism experiment
# ---------------------------

mk_high_parallelism = mk_cross_sequence(
    [ mk_parameter(experiment_key, 'high_parallelism'),
      mk_taskparts_basis,
      mk_parameters(benchmark_key, benchmarks),
      mk_schedulers,
      mk_parameter(taskparts_num_workers_key, args.num_workers) ])

# Try without binding
# -------------------

mk_core_bindings2 = mk_append(mk_parameter(taskparts_pin_worker_threads_key, 0),
                              mk_cross(
                                  mk_parameter(taskparts_pin_worker_threads_key, 1),
                                  mk_parameter(taskparts_resource_binding_key, args.binding)))

mk_taskparts_basis2 = mk_cross_sequence([
    mk_parameter(binary_key, 'sta'),
    mk_parameter(taskparts_numa_alloc_interleaved_key, 1),
    mk_core_bindings2,
    mk_parameter(infiles_path_key, path_to_infiles),])

mk_try_without_binding = mk_cross_sequence(
    [ mk_parameter(experiment_key, 'try_without_binding'),
      mk_taskparts_basis2,
      mk_parameters(benchmark_key, benchmarks),
      mk_sched_elastic_all,
      mk_parameter(taskparts_num_workers_key, args.num_workers) ])

# Parallel-sequential-mix experiment
# ----------------------------------

mix_level_key = 'mix_level_key'

mk_low_parallelism = mk_cross_sequence([
    mk_parameter('nb_repeat', 2),
    mk_parameter('nb_seq', 1),
    mk_parameter('nb_par', 1),
    mk_parameter(mix_level_key, 'low')
])

mk_med_parallelism = mk_cross_sequence([
    mk_parameter('nb_repeat', 2),
    mk_parameter('nb_seq', 1),
    mk_parameter('nb_par', 16),
    mk_parameter(mix_level_key, 'med')
])

mk_large_parallelism = mk_cross_sequence([
    mk_parameter('nb_repeat', 2),
    mk_parameter('nb_seq', 1),
    mk_parameter('nb_par', 32),
    mk_parameter(mix_level_key, 'large')
])

mk_low_med_large = mk_append_sequence([mk_low_parallelism, mk_med_parallelism, mk_large_parallelism])

mk_parallel_sequential_mix = mk_cross_sequence(
    [ mk_parameter(experiment_key, 'parallel-sequential-mix'),
      mk_taskparts_basis,
      mk_parameters(benchmark_key, benchmarks),
      mk_parameter('force_sequential', 1),
      mk_schedulers,
      mk_low_med_large,
      mk_parameter(taskparts_num_workers_key, args.num_workers),
     ])

# Vary elastic parameters experiment
# ---------------------------

mk_vary_elastic_params = mk_cross_sequence(
    [ mk_parameter(experiment_key, 'vary_elastic_params'),
      mk_taskparts_basis,
      mk_parameters(benchmark_key, benchmarks),
      mk_sched_elastic_all,
      mk_parameter(taskparts_num_workers_key, args.num_workers) ])


# Multiprogrammed work-stealing experiment
# ----------------------------------------

mk_sched_multiprogrammed = mk_cross_sequence(
    [ mk_parameter(scheduler_key, 'multiprogrammed'),
      mk_parameter(workstealing_key, 'multiprogrammed') ])

multiprogrammed_num_workers = [ args.num_workers * i for i in [1, 2, 3, 4] ]

# later: see about doing multiple comparison values, which will require dealing with additional inner joins in table generation
# https://www.w3schools.com/sql/sql_join_inner.asp

mk_multiprogrammed = mk_cross_sequence(
    [ mk_parameter(experiment_key, 'multiprogrammed'),
      mk_taskparts_basis,
      mk_parameters(benchmark_key, benchmarks),
      mk_append_sequence([
          mk_sched_nonelastic,
          mk_sched_elastic2,
          mk_sched_multiprogrammed ]),
      mk_parameters(taskparts_num_workers_key,
                    multiprogrammed_num_workers) ])

# Graph experiment
# ----------------

graph_input_key = 'input'
mk_orkut = mk_parameter(graph_input_key, 'orkut')
mk_europe = mk_cross(mk_parameter(graph_input_key, 'europe'), mk_parameter('source', 1))
# generated by ./alternatingGraph  -j -o 3 alternating.adj
mk_alternating = mk_parameter(graph_input_key, 'alternating')

mk_bfs = mk_cross(
    mk_parameter(benchmark_key, 'bfs'), mk_append(mk_orkut, mk_europe))
mk_pdfs = mk_cross(
    mk_parameter(benchmark_key, 'pdfs'), mk_append_sequence([ mk_orkut, mk_europe, mk_alternating ]))

mk_graph = mk_cross(mk_append(mk_bfs, mk_pdfs),
                    mk_cross_sequence(
                        [ mk_parameter(experiment_key, 'graph'),
                          mk_taskparts_basis,
                          mk_schedulers,
                          mk_parameter(taskparts_num_workers_key, args.num_workers) ]))


# Cilk/shootout experiment
# ------------------------

mk_cilk_shootout_schedulers = mk_parameters(workstealing_key, ['abp', 'ywra', 'cilk']) # 'cl', 

mk_cilk_shootout = mk_cross_sequence(
    [ mk_parameter(experiment_key, 'cilk_shootout'),
      mk_taskparts_basis,
      mk_parameters(benchmark_key, benchmarks),
      mk_cilk_shootout_schedulers,
      mk_parameter(taskparts_num_workers_key, args.num_workers) ])



# All experiments
# ---------------

all_experiments = { 'high_parallelism': mk_high_parallelism,
                    'parallel_sequential_mix': mk_parallel_sequential_mix,
                    'vary_elastic_params': mk_vary_elastic_params,
                    'multiprogrammed': mk_multiprogrammed, 
                    'graph': mk_graph,
                    'cilk_shootout': mk_cilk_shootout,
                    'try_without_binding': mk_try_without_binding }
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

def read_json_from_file_path(file_path):
    with open(file_path, 'r', encoding='utf-8') as fd:
        r = json.load(fd)
        fd.close()
        return r

def merge_list_of_results(rs):
    m = mk_unit()
    for r in rs:
        m = mk_append(r, m)
    return m

def merge_results_folders(rfs):
    all_results={}
    if rfs == None:
        return all_results
    for results_dirs in rfs:
        for root, dirs, files in os.walk(results_dirs):
            for f in files:
                if fnmatch.fnmatch(f, '*results*'):
                    fs = str(f)
                    if fs not in all_results:
                        all_results[fs] = mk_unit()
                    r = read_json_from_file_path(os.path.join(root, f))
                    all_results[fs] = mk_append(all_results[fs], r)
    return all_results

# later: include in the merged results all results in local_results_path
def output_merged_results():
    timestr = time.strftime("%Y-%m-%d-%H-%M-%S")
    merged_results_path = 'merged-results-' + timestr
    if args.merge_results == None:
        return None
    if not(os.path.exists(merged_results_path)):
        os.makedirs(merged_results_path)
    all_results = merge_results_folders(args.merge_results)
    for results_file in all_results:
        mf = merged_results_path + '/' + results_file
        rs = eval(all_results[results_file])
        write_json_to_file_path(rs, file_path = mf)
    return merged_results_path

def table_of_value(r):
    rows = rows_of(r)
    dicts = []
    for row in rows:
        dicts += [row_to_dictionary(row)]
    return dicts

def table_of_results(f):
    return table_of_value(read_json_from_file_path(f))

def sql_query_via_json(query, json_table,
                       output=''):
    stats_fd, stats_path = tempfile.mkstemp(suffix = '.json', text = True)
    os.close(stats_fd)
    write_json_to_file_path(json_table, stats_path)
    cmd = 'dsq ' + output + ' ' + stats_path + ' \'' + query + '\''
    current_child = subprocess.Popen(cmd, shell = True,
                                     stdout = subprocess.PIPE, stderr = subprocess.PIPE )
    child_stdout, child_stderr = current_child.communicate(timeout = 1000)
    child_stdout = child_stdout.decode('utf-8')
    child_stderr = child_stderr.decode('utf-8')
    return_code = current_child.returncode
    if return_code != 0:
        print('Error: sql_query_via_json returned error code ' + str(return_code))
        print('stdout:')
        print(str(child_stdout))
        print('stderr:')
        print(str(child_stderr))
        exit
    os.unlink(stats_path)
    json_table_out = json.loads(child_stdout) if output == '' else child_stdout
    return json_table_out

def sql_make_table_select(table, key):
    return table + '.' + key

def sql_make_table_prefix(table, key):
    return table + '_' + key

def sql_make_equality_constraint(x, y):
    return x + ' = ' + y

def sql_all_distinct_of_key(json_table, key):
    return sql_query_via_json('SELECT DISTINCT(' + key + ') FROM {}', json_table)

def sql_average_of_samples(json_table, grouping_keys, avg_keys, qfs=''):
    gs = ', '.join(grouping_keys)
    avs = ', '.join(['AVG(' + a + ') AS ' + a for a in avg_keys])
    q = 'SELECT ' + gs + ', ' + avs + ' FROM {} ' + qfs + ' GROUP BY  ' + gs
    return sql_query_via_json(q, json_table)

def sql_where_clause(conjuncts):
    return '' if conjuncts == [] else 'WHERE ' + ' AND '.join(conjuncts)

def sql_make_cross_table_binding_constraints(baseline_table, comparison_table, keys):
    return ' AND '.join([sql_make_equality_constraint(sql_make_table_select(baseline_table, k),
                                                      sql_make_table_select(comparison_table, k))
                         for k in keys])

def sql_make_comparison_table(baseline_table, comparison_table, keys, binding_kvps,
                              source_table='{}'):
    qcs = sql_make_cross_table_binding_constraints(baseline_table, comparison_table, keys)
    qbs = ' AND '.join([sql_make_equality_constraint(sql_make_table_select(comparison_table, kvp['key']), kvp['value'])
                        for kvp in binding_kvps])
    constraints = qcs + ('' if keys == [] else ' AND ') + qbs
    return 'INNER JOIN ' + source_table + ' AS ' + comparison_table + ' ON ' + constraints

def sql_make_comparison_tables(baseline_table, keys, binding_kvps,
                               source_table='{}'):
    return ' '.join([sql_make_comparison_table(baseline_table, k, keys, binding_kvps[k])
                     for k in binding_kvps])

def sql_make_comparison(baseline_table, comparison_tables, binding_kvps, keys, output_keys,
                        baseline_key, baseline_value,
                        source_table='{}'):
    fields = ','.join([sql_make_table_select(n, f) + ' AS ' + sql_make_table_prefix(n, f)
                       for f in output_keys
                       for n in comparison_tables])
    qij = sql_make_comparison_tables(baseline_table, keys, binding_kvps)
    qcs = 'WHERE ' + sql_make_equality_constraint(sql_make_table_select(baseline_table, baseline_key), baseline_value)
    return 'SELECT ' + fields + ' FROM ' + source_table + ' AS ' + baseline_table + ' ' + qij + ' ' + qcs

def sql_pctdiff(baseline_key, nonbaseline_key, key):
    return 'ROUND(100 * ((' + baseline_key + ' - ' + nonbaseline_key + ') / ' + nonbaseline_key + '), 1) AS ' + key

def sql_pctdiffs(pctdiff_keys):
    return [sql_pctdiff(sql_make_table_prefix(kvp['baseline_table'], kvp['key']),
                        sql_make_table_prefix(kvp['nonbaseline_table'], kvp['key']),
                        sql_make_table_prefix(kvp['nonbaseline_table'], kvp['key']) + '_pctdiff')
            for kvp in pctdiff_keys]    

def sql_postprocess(json_table, alias_kvps, pctdiff_keys):
    aliases = [sql_make_table_prefix(akvp['table'], akvp['key']) + ' AS ' + akvp['alias'] for akvp in alias_kvps]
    q = 'SELECT ' + ', '.join(aliases + sql_pctdiffs(pctdiff_keys)) + ' ' + ' FROM {}'
    print(sql_query_via_json(q, json_table, '--pretty'))

def sql_output_comparison(json_table, comparison_tables,
                          binding_kvps, input_keys, output_keys,
                          baseline_key, baseline_value,
                          grouping_keys, averages_keys,
                          alias_kvps, pctdiff_keys):
    baseline_table = comparison_tables[0]
    json_averages_table = sql_average_of_samples(json_table, grouping_keys, averages_keys)
    q = sql_make_comparison(baseline_table, comparison_tables, binding_kvps,
                            input_keys, output_keys, baseline_key, baseline_value)
    sql_postprocess(sql_query_via_json(q, json_averages_table), alias_kvps, pctdiff_keys)

def sql_instances_of_key(json_table, key):
    return [kvp[key] for kvp in sql_all_distinct_of_key(json_table, key)]

def sql_quote_string(s):
    return '"' + s + '"'

def experiment_table(f = 'hp.json'):
    #json_table0 = read_json_from_file_path(f)
    json_table0 = table_of_results(f)

    experiments = sql_instances_of_key(json_table0, 'experiment')
    print('========= ' + str(experiments) + ' =========')
    for experiment in experiments:
        print('---------- ' + experiment + ' ----------')
        q = 'SELECT * FROM {} ' + sql_where_clause([sql_make_equality_constraint('experiment', sql_quote_string(experiment))])
        json_table1 = sql_query_via_json(q, json_table0)
        if experiment == 'multiprogrammed':
            
            baseline_table = 'NE'
            comparison_tables = [baseline_table, 'ABP', 'S3']
            binding_kvpss = {
                'ABP': [{'key': 'scheduler', 'value': sql_quote_string('multiprogrammed')}],
                'S3': [{'key': 'scheduler', 'value': sql_quote_string('elastic2')}]}

            averages_keys = ['exectime', 'usertime']
            input_keys = ['benchmark']
            input_key_aliases = input_keys
            comparison_keys = ['scheduler']
            output_keys = input_keys + comparison_keys + averages_keys
            grouping_keys = input_keys + comparison_keys
            alias_kvps1 = [{'table': baseline_table, 'key': input_keys[i], 'alias': input_key_aliases[i]}
                           for i in range(len(input_keys))]
            alias_kvps2 = [{'table': comparison_table, 'key': key, 'alias': sql_make_table_prefix(comparison_table, key)}
                           for comparison_table in comparison_tables
                           for key in averages_keys]
            alias_kvps = alias_kvps1 + alias_kvps2
            pctdiff_keys = [{'baseline_table': baseline_table, 'nonbaseline_table': comparison_table, 'key': key}
                            for comparison_table in comparison_tables[1:]
                            for key in averages_keys]

            Ps = sql_instances_of_key(json_table0, taskparts_num_workers_key)
            for P in Ps:
                q = 'SELECT * FROM {} ' + sql_where_clause([sql_make_equality_constraint(taskparts_num_workers_key, str(P))])
                json_table = sql_query_via_json(q, json_table1)
                sql_output_comparison(json_table, comparison_tables, binding_kvpss, input_keys, output_keys,
                                      'scheduler', sql_quote_string('nonelastic'), grouping_keys, averages_keys,
                                      alias_kvps, pctdiff_keys)
                print('P = ' + str(P))
        elif experiment == 'high_parallelism':
            baseline_table = 'NE'
            comparison_tables = [baseline_table, 'S3']
            binding_kvpss = {'S3': [{'key': 'scheduler', 'value': sql_quote_string('elastic2')}]}

            averages_keys = ['exectime', 'usertime', 'nb_steals', 'nb_surplus_transitions']
            input_keys = ['benchmark']
            input_key_aliases = input_keys
            comparison_keys = ['scheduler']
            output_keys = input_keys + comparison_keys + averages_keys
            grouping_keys = input_keys + comparison_keys
            alias_kvps1 = [{'table': baseline_table, 'key': input_keys[i], 'alias': input_key_aliases[i]}
                           for i in range(len(input_keys))]
            alias_kvps2 = [{'table': comparison_table, 'key': key, 'alias': sql_make_table_prefix(comparison_table, key)}
                           for comparison_table in comparison_tables
                           for key in averages_keys]
            alias_kvps = alias_kvps1 + alias_kvps2
            pctdiff_keys = [{'baseline_table': baseline_table, 'nonbaseline_table': comparison_table, 'key': key}
                            for comparison_table in comparison_tables[1:]
                            for key in averages_keys]

            sql_output_comparison(json_table1, comparison_tables, binding_kvpss, input_keys, output_keys,
                                  'scheduler', sql_quote_string('nonelastic'), grouping_keys, averages_keys,
                                  alias_kvps, pctdiff_keys)

if args.load_results_file != None:
    for f in args.load_results_file:
        experiment_table(f)

# def pctdiff_table(f, benchmark_keys, avg_keys, comparison_key, baseline_value, nonbaseline_value, filter_kvps=[]):
#     dicts = table_of_results(f)
#     stats_fd, stats_path = tempfile.mkstemp(suffix = '.json', text = True)
#     os.close(stats_fd)
#     write_json_to_file_path(dicts, stats_path)
#     # take the average of any rows having duplicate keys wrt benchmark_keys
#     ds = ', '.join(benchmark_keys + [comparison_key])
#     avs = ', '.join(['ROUND(AVG(' + a + '),3) AS ' + a for a in avg_keys])
#     qfs = '' if filter_kvps == [] else ('WHERE ' + filter_kvps[0][0] + ' = ' + filter_kvps[0][1] + '' + ' AND '.join([fkvp[0] + ' = ' + fkvp[1] for fkvp in filter_kvps[1:]]))
#     qavs = 'SELECT ' + ds + ', ' + avs + ' FROM {} ' + qfs + ' GROUP BY  ' + ds
#     print(qavs)
#     # add columns for raw values (baseline and nonbaseline)
#     ks = ','.join(['baseline.' + a for a in benchmark_keys + avg_keys]) + ', ' + ','.join([nonbaseline_value + '.' + a + ' AS ' + a + '_' + nonbaseline_value for a in avg_keys])
#     # add columns for pctdiff fields
#     ks2 = ks + ', ' + ', '.join(['ROUND(100 * ((baseline.' + k + ' - ' + nonbaseline_value + '.' + k + ') / ' + nonbaseline_value + '.' + k + '), 1) as ' + k + '_pctdiff' for k in avg_keys])
#     js = ' AND '.join(['baseline.' + k + ' = ' + nonbaseline_value + '.' + k for k in benchmark_keys])
#     qds = 'SELECT ' + ks2 + ' FROM (' + qavs + ') ' + nonbaseline_value + ' INNER JOIN (' + qavs + ') baseline ON ' + js + ' AND baseline.' + comparison_key + ' = \"' + baseline_value + '\" WHERE baseline.' + comparison_key + ' != ' + nonbaseline_value + '.' + comparison_key
#     print(qds)
#     cmd = 'dsq --pretty ' + stats_path + ' \'' + qds + '\''
#     print(cmd)
#     current_child = subprocess.Popen(cmd, shell = True,
#                                      stdout = subprocess.PIPE, stderr = subprocess.PIPE )
#     child_stdout, child_stderr = current_child.communicate(timeout = 1000)
#     child_stdout = child_stdout.decode('utf-8')
#     child_stderr = child_stderr.decode('utf-8')
#     return_code = current_child.returncode
#     if return_code != 0:
#         print('Error: export to git returned error code ' + str(return_code))
#         print('stdout:')
#         print(str(child_stdout))
#         print('stderr:')
#         print(str(child_stderr))
#         exit
#     print(child_stdout)
#     os.unlink(stats_path)

# Driver
# ======

def export_results_to_git(local_results_path):
    if local_results_path == None:
        return
    if not(export_results):
        return
    if not(os.path.exists(local_results_path)):
        return
    bn = os.path.basename(os.path.abspath(local_results_path))
    rrp = remote_results_path + bn
    print('Exporting results in ' + local_results_path +
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
    experiment_table(results_file)
    # if e == 'high_parallelism':
    #     pctdiff_table(results_file, ['benchmark'], ['exectime', 'usertime'], 'scheduler', 'nonelastic', elastic_scheduler_value)
    # elif e == 'parallel_sequential_mix':
    #     for v in ['low', 'med', 'large']:
    #         pctdiff_table(results_file, ['benchmark'], ['exectime', 'usertime'], 'scheduler', 'nonelastic', elastic_scheduler_value, [[mix_level_key, '\"' + v + '\"']])
    #         print(v)
    # elif e == 'multiprogrammed':
    #     for p in multiprogrammed_num_workers:
    #         pctdiff_table(results_file, ['benchmark'], ['exectime', 'usertime'], 'scheduler', 'multiprogrammed', elastic_scheduler_value, [[taskparts_num_workers_key, str(p)]])
    #         print('P = ' + str(p))
    # elif e == 'try_without_binding':
    #     pctdiff_table(results_file, ['benchmark'], ['exectime', 'usertime'], taskparts_pin_worker_threads_key, '1', elastic_scheduler_value)
            

    
export_results_to_git(local_results_path)

export_results_to_git(output_merged_results())

# Original SQL query:
# dsq --pretty pctdiff_data.json 'SELECT t1.prog, t1.exectime, t1.ext as ext1, t2.ext as ext2, ROUND(100 * ((t1.exectime - t2.exectime) / t2.exectime), 1) as pctdiff FROM {} t1 INNER JOIN {} t2 ON t1.prog = t2.prog AND t2.ext = "elastic" WHERE ext1 != ext2' 

def merge_k1v1_with_values_of_k2_in_results(k1, k2, results):
    rows = rows_of(results)
    rows_out = []
    for row in rows:
        rd = row_to_dictionary(row)
        if k1 in rd and k2 in rd:
            assert k1 in rd
            rd[k1] = str(rd[k1] + '-' + rd[k2])
        row2 = dictionary_to_row(rd)
        rows_out = row2 + rows_out
    return {'value': rows_out}

# graph_results = read_json_from_file_path('results-2022-11-10-13-20-43/graph-results.json')
# graph_results2 = merge_k1v1_with_values_of_k2_in_results('experiment', 'input', graph_results)
# write_json_to_file_path(graph_results2, file_path = 'merged-results.json')

# def process_results_file():
#     if args.load_results_file == [] or args.load_results_file == None:
#         return
#     if args.split_into_files_by_keys == [] or args.split_into_files_by_keys == None:
#         return
#     f = args.load_results_file[0]
#     k = args.split_into_files_by_keys[0]
#     r = read_json_from_file_path(f)
#     rows = rows_of(r)
#     vals = set()
#     for row in rows:
#         vo = val_of_key_in_row(row, k)
#         if vo != []:
#             vals.add(vo[0])
#     bn = os.path.basename(f)
#     n, e = os.path.splitext(bn)
#     print(n)
#     print(e)
#     for v in vals:
#         n2 = n + '_' + v + e
#         print(n2)
#         vrows = []
#         for row in rows:
#             if does_row_contain_kvp(row, {'key': k, 'val': v}):
#                 vrows = vrows + [ row]
#         vr = {'value': vrows}
#         write_json_to_file_path(vr, file_path = n2)
#     print(vals)

# process_results_file()
