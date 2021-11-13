from parameter import *
from taskparts import *
from benchmark import *
from plot import *
from table import *
#from barplot import *
import itertools
import subprocess
import tabulate
from datetime import datetime
from enum import Enum

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

# if True, do not run benchmarks
virtual_runs = True
# if True, do not generate reports
virtual_report = False
# any benchmark that takes > timeout seconds gets canceled; set to None if you dislike cancel culture
timeout = 150.0 
# run minimal version of all benchmarks
class Benchmark_mode(Enum):
    Benchmark_full = 1
    Benchmark_minimal = 2
benchmark_mode = Benchmark_mode.Benchmark_minimal

def get_num_cores():
    _c = subprocess.Popen('hwloc-ls --only core | wc -l', shell = True, stdout = subprocess.PIPE)
    child_stdout, _ = _c.communicate()
    return int(child_stdout.decode('utf-8'))

max_num_workers = get_num_cores()
workers = [max_num_workers]
# workers_step = 3 if benchmark_mode == Benchmark_mode.Benchmark_full else max_num_workers
# workers = list(map(lambda x: x + 1, range(0, max_num_workers, workers_step)))
# workers = workers if 1 in workers else [1] + workers
# workers = workers if max_num_workers - 1 in workers else workers + [max_num_workers - 1]
# workers = workers if max_num_workers in workers else workers + [max_num_workers]
parallel_workers = [p for p in workers if p != 1]
num_repeat = 3 if benchmark_mode == Benchmark_mode.Benchmark_full else 1
warmup_secs = 3.0 if benchmark_mode == Benchmark_mode.Benchmark_full else 1.0

print('workers = ' + str(workers))

## Schedulers
## ----------

scheduler_key = 'scheduler'

scheduler_serial = 'serial'
scheduler_elastic_flat = 'elastic_flat'
scheduler_elastic_flat_spin = 'elastic_flat_spin'
scheduler_taskparts = 'taskparts'
elastic_schedulers = [
    scheduler_elastic_flat_spin,
    scheduler_elastic_flat,
]
taskparts_schedulers = [
    scheduler_taskparts,
] + elastic_schedulers
other_parallel_schedulers = [
#    scheduler_cilk
]
parallel_schedulers = taskparts_schedulers + other_parallel_schedulers

scheduler_descriptions = {
    scheduler_elastic_flat_spin: 'elastic (spin)',
    scheduler_elastic_flat: 'elastic (semaphore)',
    scheduler_taskparts: 'nonelastic',
}
parallel_schedulers = [s for s in [ s for s in list(scheduler_descriptions.keys()) ] if s != scheduler_serial ]

def mk_scheduler(m):
    return mk_parameter(scheduler_key, m)

## Benchmark inputs
## ----------------
 
experiment_key = 'experiment'
experiment_agac_key = 'as_good_as_conventional'
experiment_iio_key = 'impact_of_io'
experiment_ilp_key = 'impact_of_low_parallelism'
experiment_ipw_key = 'impact_of_phased_workload'
experiments = [
    experiment_agac_key,
    experiment_iio_key,
    experiment_ilp_key,
    experiment_ipw_key
]
experiments_to_run = experiments #[ experiment_iio_key, experiment_ilp_key, experiment_ipw_key ]
experiment_descriptions = {
    experiment_agac_key: 'E1',
    experiment_iio_key: 'E2',
    experiment_ilp_key: 'E3',
    experiment_ipw_key: 'E4',
}

def mk_experiment(e):
    return mk_parameter(experiment_key, e)

def mk_input(i):
    return mk_parameter('input', i)

def mk_inputs(ins):
    return mk_parameters('input', ins)

mk_agac_input = mk_experiment(experiment_agac_key)
mk_iio_input = mk_cross(mk_experiment(experiment_iio_key),
                        mk_parameter('include_infile_load', 1))
mk_ilp_input = mk_cross_sequence([mk_experiment(experiment_ilp_key),
                                  mk_parameter('override_granularity', 200)])
mk_ipw_input = mk_cross_sequence([mk_experiment(experiment_ipw_key),
                                  mk_parameter('repeat', 3)])
mk_experiments = mk_append_sequence([mk_agac_input, mk_iio_input, mk_ilp_input, mk_ipw_input])

input_descriptions = {
    'random_double': 'random',
    'exponential_double': 'exponential',
    'almost_sorted_double': 'almost sorted',
    'in_sphere': 'in sphere',
    'on_sphere': 'on sphere',
    'kuzmin': 'kuzmin',
    'random_int': 'random',
    'strings': 'strings',
    'chr22.dna': 'chr22',
    'rmat': 'rMat',
    'random': 'random'
}

# sorting inputs
# ./randomSeq -t double 10000000 random_double.seq
sequence_inputs = ['random_double'] + (['exponential_double', 'almost_sorted_double']
                                if benchmark_mode == Benchmark_mode.Benchmark_full else [])
mk_sort_input = mk_cross(mk_inputs(sequence_inputs), mk_experiments)
# convex hull inputs
# ./randPoints -S -d 2 10000000 in_sphere.geom
twod_points_inputs = ['in_sphere'] + (['on_sphere', 'kuzmin']
                                      if benchmark_mode == Benchmark_mode.Benchmark_full else [])
mk_quickhull_input = mk_cross(mk_inputs(twod_points_inputs), mk_experiments)
# removeduplicates inputs
# ./randomSeq -t int 10000000 random_int.seq
removeduplicates_inputs = ['random_int'] + (['strings']
                                            if benchmark_mode == Benchmark_mode.Benchmark_full else [])
mk_removeduplicates_input = mk_cross(mk_inputs(removeduplicates_inputs), mk_experiments)
# histogram inputs
# ./randomSeq -r 256 -t int 100000000 random_int_256.seq
histogram_inputs = ['random_int', 'random_256_int']
mk_histogram_input = mk_cross(mk_inputs(histogram_inputs), mk_experiments)
# suffixarray inputs
mk_suffixarray_input = mk_cross(mk_parameters('input', ['chr22.dna']),
                                mk_append_sequence([mk_agac_input, mk_iio_input, mk_ilp_input]))
# graph inputs
# ./rMatGraph -j 5000000 rmat.adj
# ./randLocalGraph -j 5000000 random.adj
graph_inputs = ['rmat'] + (['random']
                           if benchmark_mode == Benchmark_mode.Benchmark_full else [])
mk_graph_input = mk_cross(mk_inputs(graph_inputs), mk_experiments)
# tree inputs
mk_sum_tree_input = mk_append_sequence([mk_input(i) for i in ['perfect', 'alternating']])

## Benchmarks
## ----------

path_to_executable_key = 'path_to_executable'
benchmark_key = 'benchmark'

def mk_elastic_benchmark(b):
    return mk_parameter(benchmark_key, b)

tpal_benchmark_descriptions = {
#    'sum_array': {'input': mk_parameter('n', 10000000), 'descr': 'sum array'},
    'sum_tree': {'input': mk_sum_tree_input, 'descr': 'sum tree'},
#    'spmv': {'input': mk_parameters('input_matrix', ['bigcols','bigrows','arrowhead']), 'descr': 'sparse matrix x dense vector product'},
#    'srad': {'input': mk_unit(), 'descr': 'srad'},
    'pdfs': {'input': mk_unit(), 'descr': 'pseudo dfs'},
}
parlay_benchmark_descriptions = {
    'samplesort': {'input': mk_sort_input, 'descr': 'samplesort'},
    'quicksort': {'input': mk_sort_input, 'descr': 'quicksort'},
    'quickhull': {'input': mk_quickhull_input, 'descr': 'convex hull'},
    'removeduplicates': {'input': mk_removeduplicates_input, 'descr': 'remove duplicates'},
    'suffixarray': {'input': mk_suffixarray_input, 'descr': 'suffix array'},
#    'mis': {'input': mk_graph_input, 'descr': 'maximal independent set'},
#    'histogram': {'input': mk_histogram_input, 'descr': 'histogram'},
#    'wc': {'input': mk_unit(), 'descr': 'word count'},
#    'mcss': {'input': mk_unit(), 'descr': 'maximum contiguous subsequence sum'},
#    'integrate': {'input': mk_unit(), 'descr': 'integration'},
#    'primes': {'input': mk_unit(), 'descr': 'prime number enumeration'},
    'bfs': {'input': mk_graph_input, 'descr': 'breadth first search'},
}
benchmark_descriptions = merge_dicts(parlay_benchmark_descriptions, tpal_benchmark_descriptions)
if benchmark_mode == Benchmark_mode.Benchmark_minimal:
    takes = ['samplesort','quicksort','quickhull','removeduplicates','suffixarray','bfs']
    drops = []
else:
    takes = benchmark_descriptions
    drops = []
benchmarks = [ b for b in benchmark_descriptions if b in takes and not(b in drops) ]
mk_benchmarks = mk_append_sequence([mk_cross(mk_elastic_benchmark(b), benchmark_descriptions[b]['input'])
                                    for b in benchmarks])
mk_agac_benchmarks = mk_take_kvp(mk_benchmarks, mk_experiment(experiment_agac_key))
mk_iio_benchmarks = mk_take_kvp(mk_benchmarks, mk_experiment(experiment_iio_key))
mk_ilp_benchmarks = mk_take_kvp(mk_benchmarks, mk_experiment(experiment_ilp_key))
mk_ipw_benchmarks = mk_take_kvp(mk_benchmarks, mk_experiment(experiment_ipw_key))

benchmark_path = '../'
benchmark_bin_path = './bin/'

def is_taskparts(scheduler):
    return scheduler != scheduler_serial

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

def mk_benchmark_experiment(b, s, e):
    return mk_cross(mk_benchmark_cmd(b, s),
                    mk_take_kvp(benchmark_descriptions[b]['input'],
                                mk_experiment(e)))

def mk_benchmarks_for_experiment(e):
    return mk_append_sequence([mk_benchmark_experiment(b, s, e)
                               for b, s in cross_product(benchmarks, parallel_schedulers)])

mk_par_params = mk_cross_sequence([mk_taskparts_num_repeat(num_repeat),
                                   mk_taskparts_warmup_secs(warmup_secs)])

mk_par_agac = (mk_cross_sequence([mk_benchmarks_for_experiment(experiment_agac_key),
                                  mk_par_params,
                                  mk_parameters(taskparts_num_workers_key, workers)])
               if experiment_agac_key in experiments_to_run else mk_unit())

mk_par_iio = (mk_cross_sequence([mk_benchmarks_for_experiment(experiment_iio_key),
                                 mk_par_params,
                                 mk_parameters(taskparts_num_workers_key, parallel_workers)])
              if experiment_iio_key in experiments_to_run else mk_unit())
mk_par_ilp = (mk_cross_sequence([mk_benchmarks_for_experiment(experiment_ilp_key),
                                 mk_par_params,
                                 mk_parameters(taskparts_num_workers_key, parallel_workers)])
              if experiment_ilp_key in experiments_to_run else mk_unit())
mk_par_ipw = (mk_cross_sequence([mk_benchmarks_for_experiment(experiment_ipw_key),
                                 mk_par_params,
                                 mk_parameters(taskparts_num_workers_key, parallel_workers)])
              if experiment_ipw_key in experiments_to_run else mk_unit())

commands_parallel = mk_append_sequence([mk_par_agac, mk_par_iio, mk_par_ilp, mk_par_ipw])
    
commands = eval(commands_parallel)

## Overall benchmark setup
## -----------------------

modifiers = {
    'path_to_executable_key': path_to_executable_key,
    'outfile_keys': [taskparts_outfile_key],
    'env_vars': taskparts_env_vars,
    'silent_keys': [
        scheduler_key,
        benchmark_key,
        experiment_key
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

def percent_difference(v1, v2):
    return abs((v1 - v2) / ((v1 + v2) / 2))

def get_percent_of_one_to_another(x_key, x_val, y_expr, y_key, ref_key):
    st = mean(select_from_expr_by_key(y_expr, y_key))
    tt = mean(select_from_expr_by_key(y_expr, ref_key))
    if tt == 0.0:
        return 0.0
    return (st / tt) * 100.0

# Markdown/PDF global report
# ==========================

table_md_file = 'report.md'
table_pdf_file = 'report.pdf'

# if not(virtual_report):
#     with open(table_md_file, 'w') as f:

        # print('% Elastic scheduling benchmark report', file=f) # title
        # print('% Yue Yao, Sam Westrick, Mike Rainey, Umut Acar', file=f) # authors
        # print('% ' + datetime.now().ctime() + '\n', file=f) # date

        # print('# Preliminaries\n', file=f)
        # print('- max number of worker threads $P$ = ' + str(max_num_workers), file=f)
        # print('- all times are in seconds\n', file=f)

        # print('## Scheduler descriptions\n', file=f)
        # print(tabulate.tabulate([[k, scheduler_descriptions[k]] for k in scheduler_descriptions],
        #                         ['scheduler', 'description']) + '\n', file=f)

        # print('## Benchmark descriptions\n', file=f)
        # print(tabulate.tabulate([[k, benchmark_descriptions[k]['descr']] for k in benchmark_descriptions],
        #                         ['benchmark', 'description']) + '\n', file=f)

for experiment in experiments:
    print('\multirow{2}{*}{' + experiment_descriptions[experiment] + '} ', end = '')
    experiment_e = mk_take_kvp(all_results, mk_experiment(experiment))
    for benchmark in benchmarks:
        benchmark_inputs = mk_take_kvp(benchmark_descriptions[benchmark]['input'], mk_experiment(experiment))
        for inp in genfunc_expr_by_row(benchmark_inputs):
            print('', end = ' & ')
            pretty_input = input_descriptions[row_to_dictionary(rows_of(inp)[0])['input']]
            print(benchmark_descriptions[benchmark]['descr'] + ', ' + str(pretty_input), end = ' & ')
            benchmark_e = mk_take_kvp(experiment_e, mk_cross(mk_parameter('benchmark', benchmark),
                                                             inp))
            taskparts_scheduler_e = eval(mk_take_kvp(benchmark_e, mk_scheduler(scheduler_taskparts)))
            taskparts_exectime = mean(select_from_expr_by_key(taskparts_scheduler_e, 'exectime'))
            print(str(taskparts_exectime), end = ' & ')
            for scheduler in elastic_schedulers:
                scheduler_e = eval(mk_take_kvp(benchmark_e, mk_scheduler(scheduler)))
                scheduler_exectime = mean(select_from_expr_by_key(scheduler_e, 'exectime'))
                scheduler_diff = percent_difference(taskparts_exectime, scheduler_exectime) * 100.0
                if scheduler_exectime < taskparts_exectime:
                    scheduler_diff = scheduler_diff * -1.0
                pre = '+' if scheduler_diff > 0.0 else ''
                print(pre + "{:.1f}".format(scheduler_diff) + '\%', end = ' & ')
            taskparts_total_time = max_num_workers * taskparts_exectime
            print(str(taskparts_total_time), end = ' & ')
            n = len(list(elastic_schedulers))
            for scheduler in elastic_schedulers:
                scheduler_e = eval(mk_take_kvp(benchmark_e, mk_scheduler(scheduler)))
                scheduler_total_work_time = mean(select_from_expr_by_key(scheduler_e, 'total_work_time'))
                scheduler_total_idle_time = mean(select_from_expr_by_key(scheduler_e, 'total_idle_time'))
                scheduler_active_time = scheduler_total_work_time + scheduler_total_idle_time
                scheduler_diff = percent_difference(taskparts_total_time, scheduler_active_time) * 100.0
                if scheduler_active_time < taskparts_total_time:
                    scheduler_diff = scheduler_diff * -1.0
                pre = '+' if scheduler_diff > 0.0 else ''
                n = n - 1
                end = '' if n == 0 else ' & '
                print(pre + "{:.1f}".format(scheduler_diff) + '\%', end = end)
            print('\\\\')
    print('\hline')

    #     print('# Notes/todos\n', file=f)
    #     print('- randomize benchmark run order to avoid interactions with the OS scheduler', file=f)
    #     print('- generate report on all benchmark crashes/timeouts', file=f)
    #     print('- have the benchmark tool generate snapshots of results on a regular basis', file=f)
    #     print('- add support for building/nix based builds', file=f)
    #     print('- think of a better way to handle incomplete experiment data in plotting/tables', file=f)
   
    #     f.close()
    # process = subprocess.Popen(['pandoc', table_md_file, '-o', table_pdf_file, '--toc'],
    #                            stdout=subprocess.PIPE, 
    #                            stderr=subprocess.PIPE)
    # stdout, stderr = process.communicate()
    # print('Generated result tables in ' + table_md_file)
    # print('Generated result tables in ' + table_pdf_file)

# Safe space
    
# Time breakdown bar plots
# ========================

# if not(virtual_report):
#     def get_y_vals(expr, x_expr, y_val):
#         rs = []
#         for x_row in genfunc_expr_by_row(x_expr):
#             x_val = eval(mk_take_kvp(expr, x_row))
#             for y_row in genfunc_expr_by_row(y_val):
#                 y_key = y_row['value'][0][0]['key']
#                 ys = select_from_expr_by_key(x_val, y_key)
#                 y = 0.0
#                 if ys != []:
#                     y = mean(ys)
#                 rs += [y]
#         return rs
#     for scheduler in [scheduler_elastic_flat, scheduler_elastic_lifeline, scheduler_taskparts]:
#         barplot = mk_stacked_barplot(mk_take_kvp(mk_taskparts_num_workers(max_num_workers),
#                                                  mk_take_kvp(all_results, mk_scheduler(scheduler))),
#                                      mk_benchmarks,
#                                      mk_append_sequence([mk_parameter('total_work_time', 'number'),
#                                                          mk_parameter('total_idle_time', 'number'),
#                                                          mk_parameter('total_sleep_time', 'number')]),
#                                      get_y_vals = get_y_vals)
#         output_barplot(barplot, outfile = scheduler)

#todo: replace this function by human_readable_string_of_expr
# def string_of_expr(e, col_to_str = lambda d: str(d), row_sep = '; '):
#     rs = rows_of(e)
#     n = len(rs)
#     i = 0
#     s = ''
#     for r in rs:
#         s += col_to_str(row_to_dictionary(r))
#         if i + 1 != n:
#             s += row_sep
#     return s

# def string_of_benchmark(e):
#     col_to_str = lambda d: ';'.join(map(str, d.values()))
#     return string_of_expr(e, col_to_str)

        # def print_elastic_table(results, rows_expr, key, w, row_title):
        #     print('### $P = ' + str(w) + '$\n', file=f)
        #     print(gen_elastic_table(mk_take_kvp(results,
        #                                         mk_taskparts_num_workers(w)),
        #                             rows_expr = rows_expr, 
        #                             gen_cell = lambda row_expr, col_expr, row:
        #                             gen_table_cell_of_key(key, row),
        #                             row_title = row_title),
        #           file=f)

                # print('# As good as conventional, i.e., nonelastic\n', file=f)
        # for metric in metrics:
        #     print('## ' + metric + '\n', file=f)
        #     for w in workers:
        #         print_elastic_table(agac_results, mk_agac_benchmarks, metric, w, 'benchmark,input')

        # print('# Impact of sequential I/O\n', file=f)
        # for metric in metrics:
        #     print('## ' + metric + '\n', file=f)
        #     for w in parallel_workers:
        #         print_elastic_table(iio_results, mk_iio_benchmarks, metric, w, 'benchmark,input')

        # print('# Impact of low parallelism\n', file=f)
        # for metric in metrics:
        #     print('## ' + metric + '\n', file=f)
        #     for w in parallel_workers:
        #         print_elastic_table(ilp_results, mk_ilp_benchmarks, metric, w, 'benchmark,input,grain')

        # print('# Impact of sequential and parallel phases\n', file=f)
        # for metric in metrics:
        #     print('## ' + metric + '\n', file=f)
        #     for w in parallel_workers:
        #         print_elastic_table(ipw_results, mk_ipw_benchmarks, metric, w, 'benchmark,input,repeat')
        
        # print('# Speedup plots\n', file=f)
        # print('The baseline for each curve is the serial version of the benchmark.\n', file=f)
        # for plot in speedup_plots:
        #     plot_pdf_path = plot['plot']['default_outfile_pdf_name'] + '.pdf'
        #     print('![](' + plot_pdf_path + '){ width=100% }\n', file=f)

        # print('# Self-relative speedup plots\n', file=f)
        # print('The baseline for each curve is the taskparts parallel version of the benchmark, run on $P = 1$ cores.\n', file=f)
        # for plot in speedup_plots:
        #     plot_pdf_path = plot['plot']['default_outfile_pdf_name'] + '.pdf'
        #     print('![](' + plot_pdf_path + '){ width=100% }\n', file=f)

        # print('# Percent of time spent sleeping plots\n', file=f)
        # for plot in pct_sleeping_plots:
        #     plot_pdf_path = plot['plot']['default_outfile_pdf_name'] + '.pdf'
        #     print('![](' + plot_pdf_path + '){ width=100% }\n', file=f)

# pct_sleeping_plots = []
# if not(virtual_report):
#     pct_sleeping_plots = generate_basic_plots(
#         lambda x_key, x_val, y_expr, mk_plot_expr:
#         get_percent_of_one_to_another(x_key, x_val, y_expr, 'total_sleep_time', 'total_time'),
#         'percent of total time spent sleeping',
#         mk_append(mk_scheduler(scheduler_elastic_flat),
#                   mk_scheduler(scheduler_elastic_lifeline)),
#         ylim = [0, 100],
#         outfile_pdf_name = 'percent-of-total-time-spent-sleeping')

# def mk_get_y_val(y_key):
#     get_y_val = lambda x_key, x_val, y_expr, mk_plot_expr: mean(select_from_expr_by_key(y_expr, y_key))
#     return get_y_val

# def generate_basic_plots(get_y_val, y_label, curves_expr,
#                          ylim = [], outfile_pdf_name = ''):
#     x_label = 'workers'
#     opt_plot_args = {
#         'x_label': x_label,
#         'xlim': [1, max_num_workers + 1]
#     }
#     if ylim != []:
#         opt_plot_args['ylim'] = ylim
#     if outfile_pdf_name != '':
#         opt_plot_args['default_outfile_pdf_name'] = outfile_pdf_name
#     plots = mk_plots(all_results,
#                      mk_benchmarks,
#                      x_key = taskparts_num_workers_key,
#                      x_vals = x_vals,
#                      get_y_val = get_y_val,
#                      y_label = y_label,
#                      curves_expr = curves_expr,
#                      opt_args = opt_plot_args)
#     for plot in plots:
#         output_plot(plot)
#     return plots

# Speedup curves
# ==============

# x_vals = workers

# all_curves = mk_append_sequence([mk_scheduler(s) for s in parallel_schedulers])

# def generate_speedup_plots(mk_baseline, y_label = 'speedup'):
#     x_label = 'workers'
#     y_key = taskparts_exectime_key
#     plots = mk_speedup_plots(all_results,
#                              mk_benchmarks,
#                              max_num_workers = max_num_workers,
#                              mk_baseline = mk_baseline,
#                              x_key = taskparts_num_workers_key,
#                              x_vals = x_vals,
#                              y_key = y_key,
#                              curves_expr = all_curves)
#     for plot in plots:
#         output_plot(plot)
#     return plots

# # speedup_plots = []
# # self_relative_speedup_plots = []
# # if not(virtual_report):
# #     speedup_plots = generate_speedup_plots(mk_scheduler(scheduler_serial))
# #     mk_self_relative_baseline = mk_cross(mk_scheduler(scheduler_taskparts),
# #                                          mk_parameter(taskparts_num_workers_key, 1))
# #     self_relative_speedup_plots = generate_speedup_plots(mk_self_relative_baseline,
# #                                                          y_label = 'self relative speedup')

# Low-parallelism experiment
# ==========================

# def mean_of_key_in_row(row, result_key):
#     r  = val_of_key_in_row(row, result_key)
#     if r == []:
#         return 0.0
#     return mean(r[0])

# def gen_table_cell_of_key(key, row):
#     return mean_of_key_in_row(row, key)

# def gen_elastic_table(expr,
#                       rows_expr,
#                       gen_cell,
#                       cols_expr = mk_parameters(scheduler_key, parallel_schedulers),
#                       gen_row_str = lambda e:
#                       human_readable_string_of_expr(e,
#                                                     show_keys = False,
#                                                     silent_keys = [experiment_key,
#                                                                    'include_infile_load']),
#                       row_title = ''):
#     return gen_simple_table(mk_simple_table(expr,
#                                             mk_rows = rows_expr,
#                                             mk_cols = cols_expr,
#                                             gen_cell = gen_cell,
#                                             gen_row_str = gen_row_str,
#                                             row_title = row_title,
#                                             gen_col_titles = lambda cols_expr:
#                                             [human_readable_string_of_expr(e, show_keys = False)
#                                              for e in genfunc_expr_by_row(cols_expr)])) + '\n'    

#metrics = ['exectime', 'usertime', 'systime'] #, 'total_work_time', 'total_idle_time', 'total_sleep_time']

# agac_results = mk_take_kvp(all_results, mk_experiment(experiment_agac_key))
# iio_results = mk_take_kvp(all_results, mk_experiment(experiment_iio_key))
# ilp_results = mk_take_kvp(all_results, mk_experiment(experiment_ilp_key))
# ipw_results = mk_take_kvp(all_results, mk_experiment(experiment_ipw_key))
