import jsonschema
import simplejson as json
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

def string_of_benchmark_run(r):
    br = r['benchmark_run']
    cl_args = (' ' if br['cl_args'] != [] else '') + string_of_cl_args(br['cl_args'])
    env_args = string_of_env_args(br['env_args']) + (' ' if br['env_args'] != [] else '')
    return env_args + br['path_to_executable'] + cl_args

# Benchmark runs configuration
# ============================

path_to_executable = 'path_to_executable'

def row_to_dictionary(row):
    return dict(zip([ kvp['key'] for kvp in row ],
                    [ kvp['val'] for kvp in row ]))

def mk_benchmark_run(row, env_vars, silent_vars):
    d = row_to_dictionary(row)
    p = d[path_to_executable]
    cl_args = [ {'var': kvp['key'], 'val': kvp['val']}
                for kvp in row
                if not(kvp['key'] in ([path_to_executable] + env_vars + silent_vars)) ]
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

def mk_benchmark_runs(value, env_vars, silent_vars):
    runs = []
    for row in value['value']:
        runs += [mk_benchmark_run(row, env_vars, silent_vars)]
    r = {'runs': runs }
    jsonschema.validate(r, benchmark_run_series_schema)
    return r

# Benchmark invocation
# ====================

def dry_runs(expr, env_vars = [], silent_vars = [], output_vars = []):
    lines = ''
    value = eval(expr)
    br = mk_benchmark_runs(value, env_vars, silent_vars)
    for r in br['runs']:
        lines += string_of_benchmark_run(r) + '\n'
    return lines

