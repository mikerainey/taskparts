from taskparts_benchmark_run import *
from parameter import *

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
    return r

# br_i = run_of_row([ {"key": "benchmark", "val": "fft"},
#                     {"key": "binary", "val": "sta"},
#                     {"key": "elastic", "val": "surplus"},
#                     {"key": "n1", "val": 123},
#                     {"key": "m", "val": 456},
#                     {"key": "m2", "val": 4567},
#                     {"key": "scheduler", "val": "elastic"},
#                     {"key": taskparts_num_workers_key, "val": 16}
#                    ])
# pretty_print_json(br_i)
# validate_benchmark_run(br_i)
# #br_i = read_benchmark_from_file_path('taskparts_benchmark_run_example1.json')
# r = run_taskparts_benchmark(br_i,num_repeat=3,verbose=True,cwd='../parlay/bin')
# pretty_print_json(r)

mypath = '../parlay/'
import glob
g = glob.glob(mypath + "*.cpp")
g = [os.path.basename(x).split('.')[0] for x in g]
skips = [ 'cyclecount' ]
g = [ x for x in g if x not in skips ]
print(g)
