from plot import *
from parameter import *
from benchmark import *
from table import mk_table
import itertools
import subprocess

# Notes
# =====

# For more general-purpose table generation:
# https://pandas.pydata.org/docs/user_guide/style.html

# Taskparts configuration
# =======================

taskparts_benchmark_num_repeat_key = 'TASKPARTS_BENCHMARK_NUM_REPEAT'
taskparts_outfile_key = 'TASKPARTS_STATS_OUTFILE'
taskparts_cilk_outfile_key = 'CILK_STATS_OUTFILE'
taskparts_num_workers_key = 'TASKPARTS_NUM_WORKERS'
taskparts_exectime_key = 'exectime'
taskparts_usertime_key = 'usertime'
taskparts_systime_key = 'systime'
taskparts_maxrss_key = 'maxrss'
taskparts_total_time_key = 'total_time'
taskparts_total_work_time_key = 'total_work_time'
taskparts_total_idle_time_key = 'total_idle_time'
taskparts_total_sleep_time_key = 'total_sleep_time'
taskparts_utilization_key = 'utilization'
taskparts_nb_fibers_key = 'nb_fibers'
taskparts_nb_steals_key = 'nb_steals'

def mk_taskparts_num_repeat(n):
    return mk_parameter(taskparts_benchmark_num_repeat_key, n)

def mk_taskparts_num_workers(n):
    return mk_parameter(taskparts_num_workers_key, n)

# Experiment configuration
# ========================

## Schedulers
## ----------

scheduler_key = 'scheduler'

scheduler_serial = 'serial'
scheduler_elastic_elision = 'elastic_elision'
scheduler_elastic_lifeline = 'elastic_lifeline'
scheduler_elastic_flat = 'elastic_flat'
scheduler_taskparts = 'taskparts'
scheduler_cilk = 'cilk'
taskparts_schedulers = [
    scheduler_elastic_lifeline,
    scheduler_elastic_flat,
    scheduler_taskparts
]
other_schedulers = [
    scheduler_cilk
]
parallel_schedulers = taskparts_schedulers + other_schedulers

def mk_scheduler(m):
    return mk_parameter(scheduler_key, m)

## Benchmarks
## ----------

benchmark_key = 'benchmark'
path_to_executable_key = 'path_to_executable'

benchmarks = [
    'wc',
    'mcss',
    'samplesort',
    'suffixarray',
    'quickhull',
    #'bfs',
    # 'fib',
    # 'integrate',
    # 'primes',
    # 'removeduplicates'
]

benchmark_path = '../'
benchmark_bin_path = './bin/'

max_num_workers = 15
workers_step = 7
workers = range(1, max_num_workers + 1, workers_step)
num_repeat = 2

def binpath_of_basename(basename, scheduler = scheduler_taskparts, ext = 'sta'):
    if scheduler != 'taskparts':
        basename = basename + '.' + scheduler
    return benchmark_bin_path + basename + '.' + ext

def mk_binpath(basename, scheduler = scheduler_taskparts, ext = 'sta'):
    bp = binpath_of_basename(basename, scheduler, ext)
    return mk_parameter(path_to_executable_key, bp)

def mk_benchmark_cmd(b, scheduler = scheduler_taskparts):
    return mk_cross(mk_cross(mk_binpath(b, scheduler), mk_scheduler(scheduler)),
                    mk_parameter(benchmark_key, b))

def cross_product(xs, ys):
    return list(itertools.product(xs, ys))

commands_serial = mk_append_sequence([mk_benchmark_cmd(b, s)
                                      for b, s in cross_product(benchmarks, [scheduler_serial])])

commands_parallel = mk_cross_sequence([mk_append_sequence([mk_benchmark_cmd(b, s)
                                                           for b, s in cross_product(benchmarks, parallel_schedulers)]),
                                       mk_parameters(taskparts_num_workers_key, workers),
                                       mk_taskparts_num_repeat(num_repeat)])

commands = mk_append(commands_serial, commands_parallel)

## Overall benchmark setup
## -----------------------

modifiers = {
    'path_to_executable_key': path_to_executable_key,
    'outfile_keys': [taskparts_outfile_key,
                     taskparts_cilk_outfile_key
                     ],
    'env_vars': [
        taskparts_num_workers_key,
        taskparts_outfile_key,
        taskparts_benchmark_num_repeat_key,
        taskparts_cilk_outfile_key
    ],
    'silent_keys': [
        scheduler_key,
        benchmark_key
    ],
    'cwd': benchmark_path # cwd here because this is where the input data is stored
}

bench = mk_benchmark(commands, modifiers = modifiers)

# Preview
# =======

print('Benchmark commands to be issued:')
print('---------------')
print(string_of_benchmark_runs(bench))
print('---------------\n')

# Benchmark invocation
# ====================

#bench = step_benchmark(bench, done_peek_keys = [taskparts_exectime_key])
#add_benchmark_to_results_repository(bench)
all_results = eval(read_head_from_benchmark_repository()['done'])
print('')

# Tables
# ======

mk_benchmarks = mk_append_sequence([ mk_parameter(benchmark_key, b) for b in benchmarks ])

table_md_file = 'report.md'
table_pdf_file = 'report.pdf'

def mk_basic_table(results, result_key, columns):
    t = mk_table(results,
                 mk_benchmarks,
                 mk_parameters(scheduler_key, columns),
                 gen_row_str = lambda mk_row:
                 val_of_key_in_row(rows_of(mk_row)[0], benchmark_key),
                 gen_cell_str = lambda mk_row, mk_col, row:
                 mean(val_of_key_in_row(row, result_key)),
                 column_titles = columns) + '\n'
    return t

with open(table_md_file, 'w') as f:

    print('# Elastic scheduling benchmark report\n', file=f)
    print('Max number of worker threads $P$ = ' + str(max_num_workers) + '\n', file=f)

    results_at_scale = mk_take_kvp(all_results, mk_taskparts_num_workers(max_num_workers))
    
    print('## Raw data of runs at scale\n', file=f)
    print('### Wall-clock time\n', file=f)
    print(mk_basic_table(results_at_scale, taskparts_exectime_key, parallel_schedulers),
          file=f)
    print('### Usertime\n', file=f)
    print(mk_basic_table(results_at_scale, taskparts_usertime_key, parallel_schedulers),
          file=f)
    print('### Utilization\n', file=f)
    print(mk_basic_table(results_at_scale, taskparts_utilization_key, parallel_schedulers),
          file=f)
    print('### Total time\n', file=f)
    print(mk_basic_table(results_at_scale, taskparts_total_time_key, parallel_schedulers),
          file=f)
    print('### Total work time\n', file=f)
    print(mk_basic_table(results_at_scale, taskparts_total_work_time_key, parallel_schedulers),
          file=f)
    print('### Total idle time\n', file=f)
    print(mk_basic_table(results_at_scale, taskparts_total_idle_time_key, parallel_schedulers),
          file=f)
    print('### Total sleep time\n', file=f)
    print(mk_basic_table(results_at_scale, taskparts_total_sleep_time_key, taskparts_schedulers),
          file=f)
    print('### Number of steals\n', file=f)
    print(mk_basic_table(results_at_scale, taskparts_nb_steals_key, parallel_schedulers),
          file=f)

    f.close()

process = subprocess.Popen(['pandoc', table_md_file, '-o', table_pdf_file],
                           stdout=subprocess.PIPE, 
                           stderr=subprocess.PIPE)
stdout, stderr = process.communicate()

print('Generated result tables in ' + table_pdf_file)
    
# Speedup curves
# ==============

x_vals = workers

all_curves = mk_append_sequence([mk_scheduler(scheduler_elastic_flat),
                                 mk_scheduler(scheduler_taskparts),
                                 mk_scheduler(scheduler_cilk)])

def generate_speedup_plots():
    x_label = 'workers'
    y_key = taskparts_exectime_key
    y_label = 'speedup'
    plots = mk_speedup_plots(all_results,
                             mk_parameters(benchmark_key, benchmarks),
                             max_num_workers = max_num_workers,
                             benchmark_key = benchmark_key,
                             mk_serial_mode = mk_scheduler(scheduler_serial),
                             x_key = taskparts_num_workers_key,
                             x_vals = x_vals,
                             y_key = y_key,
                             curves_expr = all_curves)
    for plot in plots:
        output_plot(plot)

#generate_speedup_plots()

# Plots for other measures
# ========================

def percent_difference(v1, v2):
    return abs((v1 - v2) / ((v1 + v2) / 2))

def mk_get_y_val(y_key):
    get_y_val = lambda x_key, x_val, y_expr: mean(select_from_expr_by_key(y_expr, y_key))
    return get_y_val

def generate_basic_plots(get_y_val, y_label, curves_expr):
    x_label = 'workers'
    opt_plot_args = {
        'x_label': x_label,
        'xlim': [1, max_num_workers + 1]
    }
    plots = mk_plots(all_results,
                     mk_parameters(benchmark_key, benchmarks),
                     x_key = taskparts_num_workers_key,
                     x_vals = x_vals,
                     get_y_val = get_y_val,
                     y_label = y_label,
                     curves_expr = curves_expr,
                     opt_args = opt_plot_args)
    for plot in plots:
        output_plot(plot)

# generate_basic_plots(mk_get_y_val(taskparts_exectime_key), 'run time (s)', all_curves)
# generate_basic_plots(mk_get_y_val('usertime'), 'user time (s)', all_curves)
# generate_basic_plots(mk_get_y_val('systime'), 'system time (s)', all_curves)
# generate_basic_plots(mk_get_y_val('maxrss'), 'max resident size (kb)', all_curves)

def get_percent_of_one_to_another(x_key, x_val, y_expr, y_key, ref_key):
    st = mean(select_from_expr_by_key(y_expr, y_key))
    tt = mean(select_from_expr_by_key(y_expr, ref_key))
    return (st / tt) * 100.0
    
# generate_basic_plots(
#     lambda x_key, x_val, y_expr: get_percent_of_one_to_another(x_key, x_val, y_expr, 'total_sleep_time', 'total_time'),
#     'percent of total time spent sleeping',
#     mk_scheduler(scheduler_elastic_flat))
