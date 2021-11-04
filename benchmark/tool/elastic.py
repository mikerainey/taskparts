from parameter import *
from taskparts import *
from benchmark import *
from plot import *
from table import mk_table
from barplot import *
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
timeout = 50.0 # any benchmark that takes > timeout seconds gets canceled; set to None if you dislike cancel culture

max_num_workers = 15
workers_step = 3
workers = list(map(lambda x: x + 1, range(1, max_num_workers, workers_step)))
workers = workers if 1 in workers else [1] + workers
workers = workers if max_num_workers in workers else workers + [max_num_workers]
num_repeat = 3
warmup_secs = 3.0

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
def mk_elastic_benchmark(b):
    return mk_parameter(benchmark_key, b)

path_to_executable_key = 'path_to_executable'

mk_sum_tree_input = mk_append_sequence([mk_parameter('input', i) for i in ['perfect', 'alternating']])
mk_quickhull_input = mk_parameters('input', ['in_sphere', 'kuzmin'])
mk_samplesort_input = mk_parameters('input', ['random', 'exponential', 'almost_sorted'])
mk_graph_io_input = mk_cross_sequence([mk_parameter('input', 'from_file'),
                                       mk_parameter('include_graph_gen', 1),
                                       mk_parameter('infile', 'randlocal.adj')])
mk_graph_input = mk_append(mk_parameters('input', ['rMat','alternating']),
                           mk_graph_io_input)
mk_suffixarray_input = mk_parameters('infile', ['chr22.dna'])

tpal_benchmark_descriptions = {
    'sum_tree': {'input': mk_sum_tree_input, 'descr': 'sum tree'},
#    'spmv': {'input': mk_parameters('input_matrix', ['bigcols','bigrows','arrowhead']), 'descr': 'sparse matrix x dense vector product'},
    'srad': {'input': mk_unit(), 'descr': 'srad'},
    'pdfs': {'input': mk_graph_input, 'descr': 'pseudo dfs'},
}
parlay_benchmark_descriptions = {
    'wc': {'input': mk_unit(), 'descr': 'word count'},
    'mcss': {'input': mk_unit(), 'descr': 'maximum contiguous subsequence sum'},
    'samplesort': {'input': mk_samplesort_input, 'descr': 'sample sort'},
    'suffixarray': {'input': mk_suffixarray_input, 'descr': 'suffix array'},
    'quickhull': {'input': mk_quickhull_input, 'descr': 'convex hull'},
    'integrate': {'input': mk_unit(), 'descr': 'integration'},
    'primes': {'input': mk_unit(), 'descr': 'prime number enumeration'},
    'removeduplicates': {'input': mk_unit(), 'descr': 'remove duplicates'},
    'bfs': {'input': mk_graph_input, 'descr': 'breadth first search'},
}
benchmark_descriptions = merge_dicts(parlay_benchmark_descriptions, tpal_benchmark_descriptions)
#takes = ['pdfs']
#drops = []
takes = benchmark_descriptions
drops = ['primes', 'removeduplicates']
benchmarks = [ b for b in benchmark_descriptions if b in takes and not(b in drops) ]
mk_benchmarks = mk_append_sequence([mk_cross(mk_elastic_benchmark(b), benchmark_descriptions[b]['input'])
                                    for b in benchmarks])

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
                    mk_elastic_benchmark(b))

commands_serial = mk_append_sequence([mk_cross(mk_benchmark_cmd(b, s),
                                               benchmark_descriptions[b]['input'])
                                      for b, s in cross_product(benchmarks, [scheduler_serial])])
                           

commands_parallel = mk_cross_sequence([mk_append_sequence([mk_cross(mk_benchmark_cmd(b, s),
                                                                    benchmark_descriptions[b]['input'])
                                                           for b, s in cross_product(benchmarks, parallel_schedulers)]),
                                       mk_parameters(taskparts_num_workers_key, workers),
                                       mk_taskparts_num_repeat(num_repeat),
                                       mk_taskparts_warmup_secs(warmup_secs)])

commands = mk_append(commands_serial, commands_parallel)

## Overall benchmark setup
## -----------------------

modifiers = {
    'path_to_executable_key': path_to_executable_key,
    'outfile_keys': [taskparts_outfile_key,
                     taskparts_cilk_outfile_key
                     ],
    'env_vars': taskparts_env_vars,
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
    bench = step_benchmark(bench, done_peek_keys = [taskparts_exectime_key], timeout = timeout)
    add_benchmark_to_results_repository(bench)

all_results = mk_nil()
if not(virtual_report):
    all_results = eval(read_head_from_benchmark_repository()['done'])
print('')

# Time breakdown bar plots
# ========================

if not(virtual_report):
    for scheduler in [scheduler_elastic_flat, scheduler_elastic_lifeline, scheduler_taskparts]:
        def get_y_vals(expr, x_expr, y_val):
            rs = []
            for x_row in genfunc_expr_by_row(x_expr):
                x_val = eval(mk_take_kvp(expr, x_row))
                for y_row in genfunc_expr_by_row(y_val):
                    y_key = y_row['value'][0][0]['key']
                    ys = select_from_expr_by_key(x_val, y_key)
                    y = 0.0
                    if ys != []:
                        y = mean(ys)
                    rs += [y]
            return rs
        barplot = mk_stacked_barplot(mk_take_kvp(mk_taskparts_num_workers(max_num_workers),
                                                 mk_take_kvp(all_results, mk_scheduler(scheduler))),
                                     mk_benchmarks,
                                     mk_append_sequence([mk_parameter('total_work_time', 'number'),
                                                         mk_parameter('total_idle_time', 'number'),
                                                         mk_parameter('total_sleep_time', 'number')]),
                                     get_y_vals = get_y_vals)
        output_barplot(barplot, outfile = scheduler)

# Speedup curves
# ==============

x_vals = workers

all_curves = mk_append_sequence([mk_scheduler(s) for s in parallel_schedulers])

def generate_speedup_plots(mk_baseline, y_label = 'speedup'):
    x_label = 'workers'
    y_key = taskparts_exectime_key
    plots = mk_speedup_plots(all_results,
                             mk_benchmarks,
                             max_num_workers = max_num_workers,
                             mk_baseline = mk_baseline,
                             x_key = taskparts_num_workers_key,
                             x_vals = x_vals,
                             y_key = y_key,
                             curves_expr = all_curves)
    for plot in plots:
        output_plot(plot)
    return plots

speedup_plots = []
self_relative_speedup_plots = []
if not(virtual_report):
    speedup_plots = generate_speedup_plots(mk_scheduler(scheduler_serial))
    mk_self_relative_baseline = mk_cross(mk_scheduler(scheduler_taskparts),
                                         mk_parameter(taskparts_num_workers_key, 1))
    self_relative_speedup_plots = generate_speedup_plots(mk_self_relative_baseline,
                                                         y_label = 'self relative speedup')
    

# Plots for other measures
# ========================

def percent_difference(v1, v2):
    return abs((v1 - v2) / ((v1 + v2) / 2))

def mk_get_y_val(y_key):
    get_y_val = lambda x_key, x_val, y_expr, mk_plot_expr: mean(select_from_expr_by_key(y_expr, y_key))
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
                     mk_benchmarks,
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
    if tt == 0.0:
        return 0.0
    return (st / tt) * 100.0

pct_sleeping_plots = []
if not(virtual_report):
    pct_sleeping_plots = generate_basic_plots(
        lambda x_key, x_val, y_expr, mk_plot_expr:
        get_percent_of_one_to_another(x_key, x_val, y_expr, 'total_sleep_time', 'total_time'),
        'percent of total time spent sleeping',
        mk_append(mk_scheduler(scheduler_elastic_flat),
                  mk_scheduler(scheduler_elastic_lifeline)),
        ylim = [0, 100],
        outfile_pdf_name = 'percent-of-total-time-spent-sleeping')

# Markdown/PDF global report
# ==========================

def string_of_expr(e, col_to_str = lambda d: str(d), row_sep = '; '):
    rs = rows_of(e)
    n = len(rs)
    i = 0
    s = ''
    for r in rs:
        s += col_to_str(row_to_dictionary(r))
        if i + 1 != n:
            s += row_sep
    return s

def string_of_benchmark(e):
    col_to_str = lambda d: ';'.join(map(str, d.values()))
    return string_of_expr(e, col_to_str)

table_md_file = 'report.md'
table_pdf_file = 'report.pdf'

def mean_of_key_in_row(row, result_key):
    r  = val_of_key_in_row(row, result_key)
    if r == []:
        return 0.0
    return mean(r[0])

def mk_basic_table(results, result_key, columns):
    t = mk_table(results,
                 mk_benchmarks,
                 mk_parameters(scheduler_key, columns),
                 gen_row_str = string_of_benchmark,
                 gen_cell_str = lambda mk_row, mk_col, row:
                 mean_of_key_in_row(row, result_key),
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
        print(tabulate.tabulate([[k, benchmark_descriptions[k]['descr']] for k in benchmark_descriptions],
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
        print('The baseline for each curve is the serial version of the benchmark.\n', file=f)
        for plot in speedup_plots:
            plot_pdf_path = plot['plot']['default_outfile_pdf_name'] + '.pdf'
            print('![](' + plot_pdf_path + '){ width=100% }\n', file=f)

        print('# Self-relative speedup plots\n', file=f)
        print('The baseline for each curve is the taskparts parallel version of the benchmark, run on $P = 1$ cores.\n', file=f)
        for plot in speedup_plots:
            plot_pdf_path = plot['plot']['default_outfile_pdf_name'] + '.pdf'
            print('![](' + plot_pdf_path + '){ width=100% }\n', file=f)

        print('# Percent of time spent sleeping plots\n', file=f)
        for plot in pct_sleeping_plots:
            plot_pdf_path = plot['plot']['default_outfile_pdf_name'] + '.pdf'
            print('![](' + plot_pdf_path + '){ width=100% }\n', file=f)

        print('# Notes/todos\n', file=f)
        print('- implement trivial elastic policy, which resembles the algorithm used by the GHC GC... or maybe instead benchmark against variant of ABP that calls yield() after failed steal attempts', file=f)
        print('- introduce elastic + sleep by spinning?', file=f)
        print('- introduce elastic + real concurrent random set?', file=f)
        print('- randomize benchmark run order to avoid interactions with the OS scheduler', file=f)
        print('- generate report on all benchmark crashes/timeouts', file=f)
        print('- have the benchmark tool generate snapshots of results on a regular basis', file=f)
        print('- add support for building/nix based builds', file=f)
        print('- think of a better way to handle incomplete experiment data in plotting/tables', file=f)
   
        f.close()
    process = subprocess.Popen(['pandoc', table_md_file, '-o', table_pdf_file, '--toc'],
                               stdout=subprocess.PIPE, 
                               stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()
    print('Generated result tables in ' + table_md_file)
    print('Generated result tables in ' + table_pdf_file)
