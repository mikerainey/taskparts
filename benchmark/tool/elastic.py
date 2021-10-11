from plot import *
from parameter import *
from benchmark import *

def pretty_print_json(j):
    print(json.dumps(j, indent=2))

def mean(fs):
    return statistics.mean(fs)

benchmarks = [
    'wc',
    'mcss'
]

path_to_executable_key = 'path_to_executable'
taskparts_outfile_key = 'TASKPARTS_STATS_OUTFILE'
taskparts_num_workers_key = 'TASKPARTS_NUM_WORKERS'
taskparts_benchmark_num_repeat_key = 'TASKPARTS_BENCHMARK_NUM_REPEAT'
mode_key = 'mode'
benchmark_key = 'benchmark'

mode_serial = 'serial'
mode_elastic = 'elastic'
mode_taskparts = 'taksparts'
mode_cilk = 'cilk'

benchmark_bin_path = './bin/'

def benchname(basename, mode = mode_taskparts, ext = 'sta'):
    if mode != 'taskparts':
        basename = basename + '.' + mode
    return benchmark_bin_path + basename + '.' + ext

def mk_mode(m):
    return mk_parameter(mode_key, m)

def mk_elastic_benchmark(basename, mode = mode_taskparts, ext = 'sta'):
    e = mk_cross(mk_parameter(path_to_executable_key, benchname(basename, mode, ext)),
                 mk_parameter(benchmark_key, basename))
    return mk_cross(e, mk_mode(mode))
    
max_num_workers = 15
workers = range(1, max_num_workers + 1, 7)
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
        mode_key,
        benchmark_key
    ],
    'cwd': '../'
}

bench = mk_benchmark(expr, modifiers = mods)

print('Runs to be invoked:')
print(string_of_benchmark_runs(bench))

print('Invoking runs...')
bench_2 = step_benchmark(bench)

#write_benchmark_to_file_path(bench_2)
prevfname = 'results-ca5725f898ab1d09d7f29970576b600f832b70be8e06ecb7710724fdc76ad51b.json'
append_benchmark_to_file_path(bench_2, file_path_benchmark_1 = prevfname)

# with open('results.json', 'w') as fd:
#     json.dump(bench_2, fd, indent = 2)
#     fd.close()

# bench_t1 = read_benchmark_from_file_path('results-f6341bcd251547d044c2ed6e2c8defab3175ffb95b3fe4ffa7c39c080fbb58a0.json')
# bench_t2 = read_benchmark_from_file_path('results-4cb9b2cc5330f82f6d771b54d73705f080f974d93c12e1c8a11d4c33196f0c2b.json')
# bench_2 = serial_merge_benchmarks(bench_t1, bench_t2)
# write_benchmark_to_file_path(bench_2)
#pretty_print_json(bench_2)

expr = eval(bench_2['done'])
x_label = 'workers'
y_key = 'exectime'
y_label = 'speedup'
opt_plot_args = {
    "x_label": x_label,
    'xlim': [1, max_num_workers + 1],
    'ylim': [1, max_num_workers + 1]
}

plots_expr = mk_parameters(benchmark_key, benchmarks)

def get_y_val(x_key, x_val, y_expr):
    mk_b = mk_parameter(benchmark_key, select_from_expr_by_key(y_expr, benchmark_key)[0])
    mk_s = mk_mode(mode_serial)
    b = mean([float(x) for x in select_from_expr_by_key(mk_take_kvp(expr, mk_cross(mk_b, mk_s)), y_key)])
    p = mean([float(x) for x in select_from_expr_by_key(y_expr, y_key) ])
    s = b / p
    # print('')
    # pretty_print_json(y_expr)
    # print(mk_b)
    # print(b)
    # print(p)
    # print(s)
    return s

plots = mk_plots(expr,
                 plots_expr,
                 x_key = taskparts_num_workers_key, x_vals = [x for x in x_vals ],
                 get_y_val = get_y_val,
                 y_label = y_label,
                 curves_expr = mk_append(mk_mode(mode_elastic), mk_mode(mode_cilk)),
                 opt_args = opt_plot_args)
for plot in plots:
    output_plot(plot)

