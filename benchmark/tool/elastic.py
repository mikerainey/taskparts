from plot import *
from parameter import *
from benchmark import *

# Experiment configuration
# ========================

benchmarks = [
    'wc',
    'mcss',
    # 'fib',
    # 'integrate',
    # 'samplesort',
    # 'suffixarray',
    # 'primes',
    # 'quickhull',
    # 'removeduplicates'
]

path_to_executable_key = 'path_to_executable'
taskparts_outfile_key = 'TASKPARTS_STATS_OUTFILE'
taskparts_num_workers_key = 'TASKPARTS_NUM_WORKERS'
taskparts_benchmark_num_repeat_key = 'TASKPARTS_BENCHMARK_NUM_REPEAT'
mode_key = 'mode'
benchmark_key = 'benchmark'

mode_serial = 'serial'
mode_elastic = 'elastic'
mode_taskparts = 'taskparts'
mode_cilk = 'cilk'

benchmark_bin_path = './bin/'

def benchname(basename, mode = mode_taskparts, ext = 'sta'):
    if mode != 'taskparts':
        basename = basename + '.' + mode
    return benchmark_bin_path + basename + '.' + ext

def mk_mode(m):
    return mk_parameter(mode_key, m)

def mk_num_repeat(n):
    return mk_parameter(taskparts_benchmark_num_repeat_key, n)

def mk_elastic_benchmark(basename, mode = mode_taskparts, ext = 'sta'):
    e = mk_cross(mk_parameter(path_to_executable_key,
                              benchname(basename, mode, ext)),
                 mk_parameter(benchmark_key, basename))
    return mk_cross(e, mk_mode(mode))

def mk_parallel_runs(mode):
    mk_benchmarks = [mk_elastic_benchmark(b, mode = mode) for b in benchmarks]
    return mk_cross(mk_cross(mk_append_sequence(mk_benchmarks), mk_num_workers),
                    mk_num_repeat(3))

max_num_workers = 15
workers = range(1, max_num_workers + 1, 7)
x_vals = workers
mk_num_workers = mk_parameters(taskparts_num_workers_key, workers)
    
mk_serial = mk_append_sequence([mk_elastic_benchmark(b, mode = mode_serial) for b in benchmarks])
mk_taskparts = mk_parallel_runs(mode_taskparts)
mk_elastic = mk_parallel_runs(mode_elastic)
mk_cilk = mk_parallel_runs(mode_cilk)

expr = mk_append_sequence([mk_serial, mk_elastic, mk_taskparts, mk_cilk])

mods = {
    'path_to_executable_key': path_to_executable_key,
    'outfile_keys': [taskparts_outfile_key],
    'env_vars': [
        taskparts_num_workers_key,
        taskparts_outfile_key,
        taskparts_benchmark_num_repeat_key
    ],
    'silent_keys': [
        mode_key,
        benchmark_key
    ],
    'cwd': '../'
}

# Benchmark invocation
# ====================

bench = mk_benchmark(expr, modifiers = mods)

print('Runs to be invoked:')
print(string_of_benchmark_runs(bench))
print('---\n')

bench_2 = step_benchmark(bench, done_peek_keys = ['exectime'])
add_benchmark_to_results_repository(bench_2)

all_results = eval(read_head_from_benchmark_repository()['done'])

all_curves = mk_append_sequence([mk_mode(mode_elastic),
                                 mk_mode(mode_taskparts),
                                 mk_mode(mode_cilk)])

# Speedup curves
# ==============

def generate_speedup_plots():
    x_label = 'workers'
    y_key = 'exectime'
    y_label = 'speedup'
    plots = mk_speedup_plots(all_results,
                             mk_parameters(benchmark_key, benchmarks),
                             max_num_workers = max_num_workers,
                             benchmark_key = benchmark_key,
                             mk_serial_mode = mk_mode(mode_serial),
                             x_key = taskparts_num_workers_key,
                             x_vals = x_vals,
                             y_key = y_key,
                             curves_expr = all_curves)
    for plot in plots:
        output_plot(plot)

generate_speedup_plots()

# Usertime curves
# ===============

def generate_basic_plots(y_key, y_label, curves_expr):
    x_label = 'workers'
    opt_plot_args = {
        'x_label': x_label,
        'xlim': [1, max_num_workers + 1]
    }
    plots = mk_plots(all_results,
                     mk_parameters(benchmark_key, benchmarks),
                     x_key = taskparts_num_workers_key,
                     x_vals = x_vals,
                     get_y_val = lambda x_key, x_val, y_expr:
                     mean(select_from_expr_by_key(y_expr, y_key)),
                     y_label = y_label,
                     curves_expr = curves_expr,
                     opt_args = opt_plot_args)
    for plot in plots:
        output_plot(plot)
                     
generate_basic_plots('usertime', 'user time', all_curves)
generate_basic_plots('systime', 'system time', all_curves)
generate_basic_plots('maxrss', 'max resident size', all_curves)
        
# Number of steals
# ================

# later: modify Cilk RTS stats to output json
