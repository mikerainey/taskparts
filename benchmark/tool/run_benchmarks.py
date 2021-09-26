import jsonschema
import simplejson as json
import subprocess
import tempfile
import os
import matplotlib.pyplot as plt
import statistics
import functools

# String conversions
# ==================

def string_of_cl_args(args):
    out = ''
    i = 0
    for a in args:
        out = (out + ' ' + a) if i > 0 else a
        i = i + 1
    return out

def string_of_env_args(vargs):
    out = ''
    i = 0
    for va in vargs:
        v = va['var']
        a = va['val']
        o = (v + '=' + a)
        out = (out + ' ' + o) if i > 0 else o
        i = i + 1
    return out

def string_of_benchmark_run(r):
    br = r['benchmark_run']
    rargs = string_of_cl_args(br['cl_args'])
    return br['path_to_executable'] + ' ' + rargs

# Parameter constructors
# ======================

def mk_unit():
    return []

def mk_parameter(k, v):
    return [{ k: v }]

def mk_parameters(k, vs):
    r = []
    for v in vs:
        r += mk_parameter(k, str(v))
    return r

def mk_path_to_executable(p):
    return mk_parameter('path_to_executable', p)

def mk_append(ps1, ps2):
    ps = ps1.copy()
    ps.extend(ps2)
    return ps

def merge_dictionaries(d1, d2):
    d = d1.copy()
    for k in d2:
        assert(not (k in d))
        d[k] = d2[k]
    return d

def mk_cross(ps1, ps2):
    ps = []
    for i1 in range(len(ps1)):
        for i2 in range(len(ps2)):
            ps.append(merge_dictionaries(ps1[i1], ps2[i2]))
    return ps

# Benchmark constructors
# ======================

def flatten_dictionary(d):
    a = []
    for k in d.keys():
        a += [k, d[k]]
    return a

def mk_benchmark_run(p, env_var_names):
    clas = { key: value for key, value in p.items()
             if not (key in env_var_names) and key != 'path_to_executable' }
    envs = []
    for k in { key for key, value in p.items()
               if (key in env_var_names) }:
        envs += [{ 'var': k, 'val': p[k] }]
    return {
        "benchmark_run": {
            "path_to_executable": p['path_to_executable'],
            "cl_args": flatten_dictionary(clas),
            "env_args": envs
        }
    }

def mk_benchmark_runs(ps, env_var_names):
    a = []
    for p in ps:
        a += [mk_benchmark_run(p, env_var_names)]
    return {"runs": a }

# Queries on benchmark results
# ============================

def dictionary_of_env_args(eas):
    m = { }
    for a in eas:
        m[a['var']] = a['val']
    return m

def does_row_match(row, match_row):
    for key in match_row:
        if not (key in row):
            return False
        if row[key] != match_row[key]:
            return False
    return True

def filter_rows_by(rows, match_rows):
    result = []
    for row in rows:
        matches_a_row = False
        for match_row in match_rows:
            if does_row_match(row, match_row):
                matches_a_row = True
                break
        if matches_a_row:
            result += [row.copy()]
    return result

def select_by(rows, col_key):
    result = []
    for row in rows:
        if col_key in row:
            result += [row[col_key]]
    return result         

# Benchmark invocation
# ====================

with open('benchmark_run_series_schema.json', 'r') as f:
    benchmark_run_series_schema = json.loads(f.read())

results_file_name = 'results.json'
trace_file_name = 'trace.json'

# todo: in addition to env_var_names, add silent_names, which are the names of parameters
# that are not to be passed either as environment vars or command line args
def run_benchmarks(rows, env_var_names = []):
    _br = mk_benchmark_runs(rows, env_var_names)
    jsonschema.validate(_br, benchmark_run_series_schema)
    results_rows = []
    trace_rows = []
    for row in rows:
        fd, stats_file_name = tempfile.mkstemp(suffix = '.json', text = True)
        os.close(fd)
        r = mk_benchmark_run(row, env_var_names)
        cmd = string_of_benchmark_run(r)
        env_args = r['benchmark_run']['env_args']
        env_args.extend(json.loads('[{ "var": "TASKPARTS_STATS_OUTFILE", "val": "' + stats_file_name + '" }]'))
        ea_mp = dictionary_of_env_args(env_args)
        print(string_of_env_args(env_args) + ' ' + string_of_benchmark_run(r))
        current_child = subprocess.Popen(cmd, shell = True, env = ea_mp,
                                         stdout = subprocess.PIPE, stderr = subprocess.PIPE)
        so, se = current_child.communicate()
        # print(so.decode("utf-8"))
        rc = current_child.returncode
        # todo: optionally retry a benchmark run in the event of a nonzero return code
        with open(stats_file_name, 'r') as stats_file:
            stats_rows = json.load(stats_file)
            for stats_row in stats_rows:
                results_row = merge_dictionaries(row, stats_row)
                results_rows = results_rows + [results_row]
            stats_file.close()
        trace = r.copy()
        trace['return_code'] = rc
        trace_rows += [trace]
    # todo: support append to results file, and check existing results file against schema first
    with open(results_file_name, 'w') as f:
        json.dump(results_rows, f, indent = 2)
        f.close()
    # todo: print warning in case there were nonzero return codes
    with open(trace_file_name, 'w') as f:
        json.dump(trace_rows, f, indent = 2)
        f.close()

# Plotting
# ========

# todo: introduce mk_page parameter to select for multiple plots
def create_plot(rows,
                x_key = '<x axis>', x_vals = [],
                get_y_val = lambda x_key, x_val: 0.0, y_label = '<y axis>',
                mk_curve_rows = mk_unit(),
                opt_args = {}):
    curves = []
    for mk_curve_row in mk_curve_rows:
        curve_label = ','.join([ str(k) + '=' + str(v) for k, v in mk_curve_row.items() ])
        xy_pairs = []
        curve_rows = filter_rows_by(rows, [mk_curve_row])
        for x_val in x_vals:
            y_rows = filter_rows_by(curve_rows, mk_parameter(x_key, x_val))
            y_val = get_y_val(x_key, x_val, y_rows)
            xy_pairs += [{ "x": x_val, "y": y_val }]
        curves += [{"curve_label": curve_label, "xy_pairs": xy_pairs}]
    return {
        "plot": {
            "curves": curves,
            "x_label": str(x_key) if not('x_label' in opt_args) else opt_args['x_label'],
            "y_label": y_label
        }
    }

# todo: introduce pretty printing of curve labels, etc
def output_plot(plot):
    plotd = plot['plot']
    for curve in plotd['curves']:
        xy_pairs = list(curve['xy_pairs'])
        xs = list(map(lambda xyp: xyp['x'], xy_pairs))
        ys = list(map(lambda xyp: xyp['y'], xy_pairs))
        plt.plot(xs, ys, label = curve['curve_label'])
    plt.xlabel(plotd['x_label'])
    plt.ylabel(plotd['y_label'])
    plt.legend()
    plt.show()

# Misc
# ====

# todo: try plotting w/ multiple runs
x_vals = [1,4,8,12,16]
rows = mk_cross(mk_append(mk_path_to_executable('../bin/fib_oracleguided.sta'),
                          mk_path_to_executable('../bin/fib_nativeforkjoin.sta')),
                mk_parameters('TASKPARTS_NUM_WORKERS', x_vals))

run_benchmarks(rows = rows, env_var_names = ['TASKPARTS_NUM_WORKERS'])

with open(results_file_name, 'r') as f:
    rows = json.load(f)
    # print(rows)
    # print('---')
    result_rows = rows
    # print(result_rows)
    x_label = 'workers'
    y_label = 'exectime'
    opt_plot_args = {
        "x_label": x_label
    }
    plot = create_plot(rows,
                       x_key = 'TASKPARTS_NUM_WORKERS',
                       x_vals = [str(x) for x in x_vals ],
                       get_y_val =
                       lambda x_key, x_val, y_rows:
                       statistics.mean([float(x) for x in select_by(y_rows, y_label) ]),
                       y_label = y_label,
                       mk_curve_rows = mk_append(mk_path_to_executable('../bin/fib_oracleguided.sta'),
                                                 mk_path_to_executable('../bin/fib_nativeforkjoin.sta')),
                       opt_args = opt_plot_args)
    print(plot)
    output_plot(plot)
    f.close()
