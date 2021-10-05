from plot import *
from parameter import *
from benchmark import *

x_vals = range(1, 16)
max_num_workers = max(x_vals)
kappa_usecs = [200]
taskparts_kappa_usec_key = 'TASKPARTS_KAPPA_USEC_KEY'
taskparts_outfile_key = 'TASKPARTS_STATS_OUTFILE'
taskparts_num_workers_key = 'TASKPARTS_NUM_WORKERS'
taskparts_benchmark_num_repeat_key = 'TASKPARTS_BENCHMARK_NUM_REPEAT'

mk_serial = mk_parameter('path_to_executable', './fib.serial.sta')

mk_oracleguided = mk_cross(mk_parameter('path_to_executable', './fib_oracleguided.sta'),
                           mk_parameters(taskparts_kappa_usec_key, kappa_usecs))

mk_nativeforkjoin = mk_parameter('path_to_executable', './fib_nativeforkjoin.sta')

q = mk_append(mk_serial,
              mk_cross(mk_cross(mk_append(mk_oracleguided, mk_nativeforkjoin),
                                mk_parameters(taskparts_num_workers_key, x_vals)),
                       mk_parameter(taskparts_benchmark_num_repeat_key, 3)))
env_vars = [taskparts_num_workers_key, taskparts_outfile_key, taskparts_kappa_usec_key, taskparts_benchmark_num_repeat_key]
mods = {
    'path_to_executable_key': 'path_to_executable',
    'outfile_keys': [taskparts_outfile_key],
    'env_vars': env_vars,
    'cwd': '../bin/'
}
bench = mk_benchmark(q, modifiers = mods)

print('--- Todo:')
print(string_of_benchmark_runs(bench))
print('---')

bench_2 = step_benchmark(bench)

expr = eval(bench_2['done'])
x_label = 'workers'
y_key = 'exectime'
y_label = 'speedup'
opt_plot_args = {
    "x_label": x_label,
    'title': 'Speedup curves for fib(44)',
    'default_outfile_pdf_name': 'fib44',
    'xlim': [1, max_num_workers + 1],
    'ylim': [1, max_num_workers + 1]
}

def get_y_val(x_key, x_val, y_expr):
    b = statistics.mean([float(x) for x in select_from_expr_by_key(mk_take_kvp(expr, mk_serial), y_key)])
    return b / statistics.mean([float(x) for x in select_from_expr_by_key(y_expr, y_key) ])

plot = mk_plot(expr,
               x_key = taskparts_num_workers_key, x_vals = [x for x in x_vals ],
               get_y_val = get_y_val,
               y_label = y_label,
               curves_expr = mk_append(mk_oracleguided, mk_nativeforkjoin),
               opt_args = opt_plot_args)
output_plot(plot)

