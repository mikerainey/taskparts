from plot import *
from parameter import *
from taskparts import *
from benchmark import *
from table import mk_table
import itertools
import subprocess
import tabulate
from datetime import datetime

def merge_dicts(x, y):
    return { **x, **y }

def cross_product(xs, ys):
    return list(itertools.product(xs, ys))

# Notes
# =====

# For more general-purpose table generation:
# https://pandas.pydata.org/docs/user_guide/style.html

# Experiment configuration
# ========================

virtual_runs = True # if True, do not run benchmarks
virtual_report = False # if True, do not generate reports

max_num_workers = 15
workers_step = 7
workers = range(1, max_num_workers + 1, workers_step)
num_repeat = 2

## Schedulers
## ----------

scheduler_key = 'scheduler'

scheduler_serial = 'serial'
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

scheduler_descriptions = {
    scheduler_serial: 'serial',
    scheduler_elastic_lifeline: 'elastic (lifelines)',
    scheduler_elastic_flat: 'elastic (no lifelines)',
    scheduler_taskparts: 'taskparts (Chase Lev work stealing / no elastic)',
    scheduler_cilk: 'Cilk Plus (gcc7)',
}

def mk_scheduler(m):
    return mk_parameter(scheduler_key, m)

## Benchmarks
## ----------

benchmark_key = 'benchmark'
path_to_executable_key = 'path_to_executable'

tpal_benchmark_descriptions = {
    'sum_array': 'sum array',
    'sum_tree': 'sum tree',
    'spmv': 'sparse matrix x dense vector product',
    'srad': 'srad',
#    'pdfs': 'pseudo dfs'
}
parlay_benchmark_descriptions = {
    'wc': 'word count',
    'mcss': 'maximum contiguous subsequence sum',
    'samplesort': 'sample sort',
    'suffixarray': 'suffix array',
    'quickhull': 'convex hull',
    'integrate': 'integration',
    'primes': 'prime number enumeration',
    'removeduplicates': 'remove duplicates'
}
benchmark_descriptions = merge_dicts(parlay_benchmark_descriptions, tpal_benchmark_descriptions)
takes = ['wc', 'mcss', 'samplesort', 'quickhull', 'primes', 'removeduplicates'] # [b for b in benchmark_descriptions]
takes = ['integrate', 'sum_array']
drops = []
benchmarks = [ b for b in benchmark_descriptions if b in takes and not(b in drops) ]
mk_benchmarks = mk_parameters(benchmark_key, benchmarks)

benchmark_path = '../'
benchmark_bin_path = './bin/'

def is_taskparts(scheduler):
    return scheduler != scheduler_cilk and scheduler != scheduler_serial

def is_tpal_binary(basename, scheduler):
    return (basename in tpal_benchmark_descriptions) and is_taskparts(scheduler)

def binpath_of_basename(basename, scheduler = scheduler_taskparts, ext = 'sta'):
    if is_tpal_binary(basename, scheduler):
        scheduler = scheduler + '_tpal'
    basename = basename + '.' + scheduler
    return benchmark_bin_path + basename + '.' + ext

def mk_binpath(basename, scheduler = scheduler_taskparts, ext = 'sta'):
    bp = binpath_of_basename(basename, scheduler, ext)
    return mk_parameter(path_to_executable_key, bp)

def mk_benchmark_cmd(b, scheduler = scheduler_taskparts):
    return mk_cross(mk_cross(mk_binpath(b, scheduler), mk_scheduler(scheduler)),
                    mk_parameter(benchmark_key, b))

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

if not(virtual_runs):
    bench = step_benchmark(bench, done_peek_keys = [taskparts_exectime_key])
    add_benchmark_to_results_repository(bench)

all_results = mk_nil()
if not(virtual_report):
    all_results = eval(read_head_from_benchmark_repository()['done'])
print('')

# Speedup curves
# ==============

x_vals = workers

all_curves = mk_append_sequence([mk_scheduler(s) for s in parallel_schedulers])

def generate_speedup_plots():
    x_label = 'workers'
    y_key = taskparts_exectime_key
    y_label = 'speedup'
    plots = mk_speedup_plots(all_results,
                             mk_benchmarks,
                             max_num_workers = max_num_workers,
                             benchmark_key = benchmark_key,
                             mk_serial_mode = mk_scheduler(scheduler_serial),
                             x_key = taskparts_num_workers_key,
                             x_vals = x_vals,
                             y_key = y_key,
                             curves_expr = all_curves)
    for plot in plots:
        output_plot(plot)
    return plots

speedup_plots = []
if not(virtual_report):
    speedup_plots = generate_speedup_plots()

# Plots for other measures
# ========================

def percent_difference(v1, v2):
    return abs((v1 - v2) / ((v1 + v2) / 2))

def mk_get_y_val(y_key):
    get_y_val = lambda x_key, x_val, y_expr: mean(select_from_expr_by_key(y_expr, y_key))
    return get_y_val

def generate_basic_plots(get_y_val, y_label, curves_expr,
                         ylim = [], outfile_pdf_name = ''):
    x_label = 'workers'
    opt_plot_args = {
        'x_label': x_label,
        'xlim': [1, max_num_workers + 1]
    }
    if ylim != []:
        opt_plot_args['ylim'] = ylim
    if outfile_pdf_name != '':
        opt_plot_args['default_outfile_pdf_name'] = outfile_pdf_name
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
    return plots

def get_percent_of_one_to_another(x_key, x_val, y_expr, y_key, ref_key):
    st = mean(select_from_expr_by_key(y_expr, y_key))
    tt = mean(select_from_expr_by_key(y_expr, ref_key))
    return (st / tt) * 100.0

pct_sleeping_plots = []
if not(virtual_report):
    pct_sleeping_plots = generate_basic_plots(
        lambda x_key, x_val, y_expr:
        get_percent_of_one_to_another(x_key, x_val, y_expr, 'total_sleep_time', 'total_time'),
        'pct. of total time spent sleeping',
        mk_append(mk_scheduler(scheduler_elastic_flat),
                  mk_scheduler(scheduler_elastic_lifeline)),
        ylim = [0, 100],
        outfile_pdf_name = 'percent-of-total-time-spent-sleeping')

# Markdown/PDF global report
# ==========================

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

if not(virtual_report):
    with open(table_md_file, 'w') as f:

        print('% Elastic scheduling benchmark report', file=f) # title
        print('% Yue Yao, Sam Westrick, Mike Rainey, Umut Acar', file=f) # authors
        print('% ' + datetime.now().ctime() + '\n', file=f) # date

        print('# Preliminaries\n', file=f)
        print('- max number of worker threads $P$ = ' + str(max_num_workers), file=f)
        print('- all times are in seconds\n', file=f)

        print('## Scheduler descriptions\n', file=f)
        print(tabulate.tabulate([[k, scheduler_descriptions[k]] for k in scheduler_descriptions],
                                ['scheduler', 'description']) + '\n', file=f)

        print('## Benchmark descriptions\n', file=f)
        print(tabulate.tabulate([[k, benchmark_descriptions[k]] for k in benchmark_descriptions],
                                ['benchmark', 'description']) + '\n', file=f)

        results_at_scale = mk_take_kvp(all_results, mk_taskparts_num_workers(max_num_workers))

        print('# Raw data of runs at scale\n', file=f)
        print('## Wall-clock time\n', file=f)
        print(mk_basic_table(results_at_scale, taskparts_exectime_key, parallel_schedulers),
              file=f)
        print('## Total time\n', file=f)
        print(mk_basic_table(results_at_scale, taskparts_total_time_key, parallel_schedulers),
              file=f)
        print('## Total work time\n', file=f)
        print(mk_basic_table(results_at_scale, taskparts_total_work_time_key, parallel_schedulers),
              file=f)
        print('## Total idle time\n', file=f)
        print(mk_basic_table(results_at_scale, taskparts_total_idle_time_key, parallel_schedulers),
              file=f)
        print('## Total sleep time\n', file=f)
        print(mk_basic_table(results_at_scale, taskparts_total_sleep_time_key, taskparts_schedulers),
              file=f)

        print('## User time\n', file=f)
        print(mk_basic_table(results_at_scale, taskparts_usertime_key, parallel_schedulers),
              file=f)
        print('## System time\n', file=f)
        print(mk_basic_table(results_at_scale, taskparts_systime_key, parallel_schedulers),
              file=f)
        print('## Utilization (percent of total time / 100)\n', file=f)
        print(mk_basic_table(results_at_scale, taskparts_utilization_key, parallel_schedulers),
              file=f)
        print('## Number of steals\n', file=f)
        print(mk_basic_table(results_at_scale, taskparts_nb_steals_key, parallel_schedulers),
              file=f)
        print('## Maximum resident set size\n', file=f)
        print(mk_basic_table(results_at_scale, taskparts_maxrss_key, parallel_schedulers),
              file=f)

        print('# Speedup plots\n', file=f)
        for plot in speedup_plots:
            plot_pdf_path = plot['plot']['default_outfile_pdf_name'] + '.pdf'
            print('![](' + plot_pdf_path + '){ width=100% }\n', file=f)

        print('# Percent of time spent sleeping plots\n', file=f)
        for plot in pct_sleeping_plots:
            plot_pdf_path = plot['plot']['default_outfile_pdf_name'] + '.pdf'
            print('![](' + plot_pdf_path + '){ width=100% }\n', file=f)

        print('# Notes/todos\n', file=f)
        print('- deal with crashing bfs.cilk by setting warmup time to zero', file=f)
        print('- generate challenge graphs/trees for pdfs/sum_tree resp.', file=f)
        print('- implement trivial elastic policy, which resembles the algorithm used by the GHC GC', file=f)
        print('- introduce elastic + sleep by spinning?', file=f)
        print('- introduce elastic + real concurrent random set?', file=f)
        print('- better plot titles', file=f)

        f.close()

if not(virtual_report):
    process = subprocess.Popen(['pandoc', table_md_file, '-o', table_pdf_file, '--toc'],
                               stdout=subprocess.PIPE, 
                               stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()
    print('Generated result tables in ' + table_pdf_file)
