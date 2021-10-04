from plot import *
from parameter import *
from benchmark import *

def pretty_print_json(j):
    print(json.dumps(j, indent=2))

def mean(fs):
    return statistics.mean(fs)

benchmarks = [
    'fib'
]

path_to_executable_key = 'path_to_executable'
taskparts_outfile_key = 'TASKPARTS_STATS_OUTFILE'
taskparts_num_workers_key = 'TASKPARTS_NUM_WORKERS'
taskparts_benchmark_num_repeat_key = 'TASKPARTS_BENCHMARK_NUM_REPEAT'
mode_key = 'mode'

mode_serial = 'serial'
mode_elastic = 'elastic'
mode_taskparts = 'taksparts'
mode_cilk = 'cilk'

benchmark_bin_path = './bin/'

def benchname(basename, mode = mode_taskparts, ext = 'sta'):
    if mode != 'taskparts':
        basename = basename + '.' + mode
    return benchmark_bin_path + basename + '.' + ext

def mk_elastic_benchmark(basename, mode = mode_taskparts, ext = 'sta'):
    e = mk_parameter(path_to_executable_key, benchname(basename, mode, ext))
    se = mk_parameter(mode_key, mode)
    return mk_cross(e, se)
    
max_num_workers = 15
workers = [15] #range(1, max_num_workers + 1)
x_vals = workers
mk_num_workers = mk_parameters(taskparts_num_workers_key, workers)
    
mk_serial = mk_append_sequence([mk_elastic_benchmark(b, mode = mode_serial) for b in benchmarks])
mk_elastic = mk_cross(mk_append_sequence([mk_elastic_benchmark(b, mode = mode_elastic) for b in benchmarks]), mk_num_workers)
mk_cilk = mk_cross(mk_append_sequence([mk_elastic_benchmark(b, mode = mode_cilk) for b in benchmarks]), mk_num_workers)

expr = mk_append_sequence([mk_serial, mk_elastic, mk_cilk])

mods = {
    'path_to_executable_key': 'path_to_executable',
    'outfile_keys': [taskparts_outfile_key],
    'env_vars': [
        taskparts_num_workers_key,
        taskparts_outfile_key,
        taskparts_benchmark_num_repeat_key
    ],
    'silent_keys': [
        mode_key
    ],
    'cwd': '../'
}

bench = mk_benchmark(expr, modifiers = mods)

print('Runs to be invoked:')
print(string_of_benchmark_runs(bench))

print('Invoking runs...')
bench_2 = step_benchmark(bench)

pretty_print_json(bench_2)

# expr = eval(bench_2['done'])
# x_label = 'workers'
# y_key = 'exectime'
# y_label = 'speedup'
# opt_plot_args = {
#     "x_label": x_label
# }

# def get_y_val(x_key, x_val, y_expr):
#     b = mean([float(x) for x in select_from_expr_by_key(mk_take_kvp(expr, mk_serial), y_key)])
#     return b / mean([float(x) for x in select_from_expr_by_key(y_expr, y_key) ])

# plot = mk_plot(expr,
#                x_key = taskparts_num_workers_key, x_vals = [x for x in x_vals ],
#                get_y_val = get_y_val,
#                y_label = y_label,
#                curves_expr = mk_append(mk_oracleguided, mk_nativeforkjoin),
#                opt_args = opt_plot_args)
# output_plot(plot, results_plot_fname = 'plot.pdf')

