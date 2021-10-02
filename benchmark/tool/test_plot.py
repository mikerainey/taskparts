from plot import *
from parameter import *
from benchmark import *

# todo: try plotting w/ multiple runs
x_vals = [1,8,12,16] #,8,12,16
kappa_usecs = [200,300,400]
taskparts_kappa_usec_key = 'TASKPARTS_KAPPA_USEC_KEY'
taskparts_outfile_key = 'TASKPARTS_STATS_OUTFILE'
taskparts_num_workers_key = 'TASKPARTS_NUM_WORKERS'

mk_oracleguided = mk_cross(mk_parameter(path_to_executable, '../bin/fib_oracleguided.sta'),
                           mk_parameters(taskparts_kappa_usec_key, kappa_usecs))

mk_nativeforkjoin = mk_parameter(path_to_executable, '../bin/fib_nativeforkjoin.sta')


q = mk_cross(mk_append(mk_oracleguided, mk_nativeforkjoin),
             mk_parameters(taskparts_num_workers_key, x_vals))
env_vars = [taskparts_num_workers_key, taskparts_outfile_key, taskparts_kappa_usec_key]
do_benchmark_runs(q, env_vars=env_vars, outfile_keys = [taskparts_outfile_key], append_output = False)

results_file_name = 'results.json'
with open(results_file_name, 'r') as f:
    expr = eval(json.load(f))
    x_label = 'workers'
    y_label = 'exectime'
    opt_plot_args = {
        "x_label": x_label
    }
    plot = create_plot(expr,
                       x_key = taskparts_num_workers_key, x_vals = [x for x in x_vals ],
                       get_y_val =
                       lambda x_key, x_val, y_expr:
                       statistics.mean([float(x) for x in select_from_expr_by_key(y_expr, y_label) ]),
                       y_label = y_label,
                       curves_expr = mk_append(mk_oracleguided, mk_nativeforkjoin),
                       opt_args = opt_plot_args)
    output_plot(plot, results_plot_name = 'plot.pdf')
    f.close()
