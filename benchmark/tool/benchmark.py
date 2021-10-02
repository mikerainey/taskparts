import jsonschema
import simplejson as json
import subprocess
import tempfile
import os
import sys
from parameter import *

# String conversions
# ==================

def string_of_cl_args(args):
    out = ''
    i = 0
    for a in args:
        sa = "-" + a['var'] + ' ' + str(a['val'])
        out = (out + ' ' + sa) if i > 0 else sa
        i = i + 1
    return out

def string_of_env_args(vargs):
    out = ''
    i = 0
    for va in vargs:
        v = va['var']
        sa = str(va['val'])
        o = (v + '=' + sa)
        out = (out + ' ' + o) if i > 0 else o
        i = i + 1
    return out

def string_of_benchmark_run(r, show_env_args = False):
    br = r['benchmark_run']
    cl_args = (' ' if br['cl_args'] != [] else '') + string_of_cl_args(br['cl_args'])
    env_args = ''
    if show_env_args:
        env_args = string_of_env_args(br['env_args']) + (' ' if br['env_args'] != [] else '')
    return env_args + br['path_to_executable'] + cl_args

# Benchmark runs configuration
# ============================

# reserved keys:
path_to_executable = 'path_to_executable'
cwd = 'cwd'

def row_to_dictionary(row):
    return dict(zip([ kvp['key'] for kvp in row ],
                    [ kvp['val'] for kvp in row ]))

def dictionary_to_row(dct):
    return [ {'key': k, 'val': v} for k, v in dct.items() ]

def mk_benchmark_run(row, env_vars):
    d = row_to_dictionary(row)
    p = d[path_to_executable]
    cl_args = [ {'var': kvp['key'], 'val': kvp['val']}
                for kvp in row
                if not(kvp['key'] in ([path_to_executable] + env_vars)) ]
    env_args = [ {'var': kvp['key'], 'val': kvp['val']}
                 for kvp in row if kvp['key'] in env_vars ]
    return {
        "benchmark_run": {
            "path_to_executable": p,
            "cl_args": cl_args,
            "env_args": env_args
        }
    }

with open('benchmark_run_series_schema.json', 'r') as f:
    benchmark_run_series_schema = json.loads(f.read())

def mk_benchmark_runs(value, env_vars):
    runs = []
    for row in value['value']:
        runs += [mk_benchmark_run(row, env_vars)]
    r = {'runs': runs }
    jsonschema.validate(r, benchmark_run_series_schema)
    return r

# Benchmark invocation
# ====================

def extend_expr_with_output_file_targets(expr, outfile_keys, gen_tmpfiles = True):
    tmpfiles = []
    for ofn in outfile_keys:
        ofv = '<temp file name>'
        if gen_tmpfiles:
            fd, ofv = tempfile.mkstemp(suffix = '.json', text = True)
            tmpfiles += [{'key': ofn, 'val': ofv}]
            os.close(fd)
        expr = mk_cross(expr, mk_parameter(ofn, ofv))
    return expr, tmpfiles

# later: allow key specified by the cwd variable above to change current directory too its value
def string_of_dry_runs(expr, env_vars = [], outfile_keys = []):
    lines = ''
    expr, _ = extend_expr_with_output_file_targets(expr, outfile_keys, gen_tmpfiles = False)
    value = eval(expr)
    brs = mk_benchmark_runs(value, env_vars)
    i = 0
    for r in brs['runs']:
        sbr = string_of_benchmark_run(r, show_env_args = True)
        lines += '[' + str(i) + '] ' + sbr + '\n'
        i += 1
    return lines

# todo: support cwd
def do_benchmark_runs(expr, env_vars = [], outfile_keys = [],
                      results_fname = 'results.json', trace_fname = 'trace.json',
                      append_output = False,
                      client_format_to_row = lambda d: dictionary_to_row(d),
                      dry_run = False, verbose = True):
    if dry_run:
        print(string_of_dry_runs(expr, env_vars, outfile_keys))
        return []
    results_fd = sys.stdout
    trace_fd = open(trace_fname, 'a+')
    if not(append_output):
        open(results_fname, 'w').close() 
        results_fd = open(results_fname, 'w')
        results_rows = []
    else:
        results_fd = open(results_fname, 'r')
        old_results = json.load(results_fd)
        jsonschema.validate(old_results, parameter_schema)
        results_rows = old_results['value'] if len(old_results['value']) > 0 else []
        open(results_fname, 'w').close() 
        results_fd = open(results_fname, 'w')
    trace_rows = []
    expr, tmpfiles = extend_expr_with_output_file_targets(expr, outfile_keys)
    value = eval(expr)
    _ = mk_benchmark_runs(value, env_vars) # for json schema validation
    for input_row in value['value']:
        run = mk_benchmark_run(input_row, env_vars)
        if verbose:
            print(string_of_benchmark_run(run, show_env_args = True))
        cmd = string_of_benchmark_run(run)
        env_args_dict = { a['var']: str(a['val']) for a in run['benchmark_run']['env_args'] }
        current_child = subprocess.Popen(cmd, shell = True, env = env_args_dict,
                                         stdout = subprocess.PIPE, stderr = subprocess.PIPE)
        so, se = current_child.communicate()
        rc = current_child.returncode
        results_row = []
        for tf in tmpfiles:
            tfn = tf['val']
            tfd = open(tfn, 'r')
            j = json.load(tfd)
            open(tfn, 'w').close()
            results_row += eval(mk_cross({'value': [input_row]},
                                         {'value': [client_format_to_row(d) for d in j] }))['value']
        results_rows += results_row
        trace = run.copy()
        trace['return_code'] = rc
        trace_rows += [trace]
    for tf in tmpfiles:
        os.unlink(tf['val'])
    results_val = {'value': results_rows}
    jsonschema.validate(results_val, parameter_schema)
    json.dump(results_val, results_fd, indent = 2)
    results_fd.close()
    json.dump(trace_rows, trace_fd, indent = 2)
    trace_fd.close()
    return results_val
