import jsonschema
import simplejson as json
import subprocess
import tempfile
import os
import sys
from parameter import *

with open('benchmark_schema.json', 'r') as f:
    benchmark_schema = json.loads(f.read())

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

# Benchmark stepper
# =================

def row_to_dictionary(row):
    return dict(zip([ kvp['key'] for kvp in row ],
                    [ kvp['val'] for kvp in row ]))

def dictionary_to_row(dct):
    return [ {'key': k, 'val': v} for k, v in dct.items() ]

def mk_benchmark_run(row, env_vars, path_to_executable_key):
    d = row_to_dictionary(row)
    p = d[path_to_executable_key]
    cl_args = [ {'var': kvp['key'], 'val': kvp['val']}
                for kvp in row
                if not(kvp['key'] in ([path_to_executable_key] + env_vars)) ]
    env_args = [ {'var': kvp['key'], 'val': kvp['val']}
                 for kvp in row if kvp['key'] in env_vars ]
    return {
        'benchmark_run': {
            'path_to_executable': p,
            'cl_args': cl_args,
            'env_args': env_args
        }
    }

def mk_outfile_keys(outfiles):
    e = mk_unit()
    for r in outfiles:
        e = mk_append(e, mk_parameter(r['key'], r['file_name']))
    return e

def mk_benchmark(parameters,
                 modifiers = {
                     'path_to_executable_key': 'path_to_executable',
                     'outfile_keys': []
                 },
                 todo = mk_unit(), trace = [], done = mk_unit()):
    b = {'parameters': parameters, 'modifiers': modifiers, 'todo': todo, 'trace': [], 'done': done}
    jsonschema.validate(b, benchmark_schema)
    return b

def nb_todo_in_benchmark(b):
    return len(b['todo']['value'])

def seed_benchmark(benchmark_1):
    benchmark_2 = benchmark_1.copy()
    assert(get_first_key_in_dictionary(benchmark_2['todo']) == 'value')
    if nb_todo_in_benchmark(benchmark_2) == 0:
        benchmark_2['todo'] = eval(benchmark_1['parameters'])
    return benchmark_2    

def step_benchmark_1(benchmark_1, verbose = True):
    jsonschema.validate(benchmark_1, benchmark_schema)
    parameters = benchmark_1['parameters']
    modifiers = benchmark_1['modifiers']
    # Try to load the next run to do
    benchmark_2 = seed_benchmark(benchmark_1)
    todo = benchmark_2['todo']
    if todo['value'] == []:
        jsonschema.validate(benchmark_2, benchmark_schema)
        return benchmark_2
    # Create any tempfiles needed to collect results
    outfile_keys = modifiers['outfile_keys']
    outfiles = []
    for k in outfile_keys:
        fd, n = tempfile.mkstemp(suffix = '.json', text = True)
        outfiles += [{'key': k, 'file_name': n, 'fd': fd}]
        os.close(fd)
    # Fetch the next run
    next_row = eval(mk_cross(todo, mk_outfile_keys(outfiles)))['value'].pop()
    for r in outfiles:
        os.unlink(r['file_name'])
    todo_2 = todo['value'].copy()
    todo_2.pop()
    env_vars = modifiers['env_vars'] if 'env_vars' in modifiers else []
    next_run = mk_benchmark_run(next_row, env_vars, modifiers['path_to_executable_key'])
    # Print
    if verbose:
        print(string_of_benchmark_run(next_run, show_env_args = True))
    # Do the next run
    # todo: handle cwd
    cmd = string_of_benchmark_run(next_run)
    env_args_dict = { a['var']: str(a['val']) for a in next_run['benchmark_run']['env_args'] }
    current_child = subprocess.Popen(cmd, shell = True, env = env_args_dict,
                                     stdout = subprocess.PIPE, stderr = subprocess.PIPE)
    so, se = current_child.communicate()
    rc = current_child.returncode
    # Collect the results
    results_expr = {'value': [ next_row ]}
    for of in outfiles:
        fd = open(of['file_name'], 'r')
        j = json.load(fd)
        open(of['file_name'], 'w').close()
        results_expr = mk_cross(results_expr, {'value': [dictionary_to_row(d) for d in j] })
    # Save the results
    trace_2 = next_run.copy()
    trace_2['benchmark_run']['return_code'] = rc
    benchmark_2['todo'] = { 'value': todo_2 }
    benchmark_2['trace'] += [ trace_2 ]
    benchmark_2['done'] = eval(mk_append(benchmark_1['done'], results_expr))
    jsonschema.validate(benchmark_2, benchmark_schema)
    return benchmark_2

def step_benchmark(benchmark_1):
    benchmark_2 = seed_benchmark(benchmark_1)
    nb_todo = nb_todo_in_benchmark(benchmark_2)
    while nb_todo > 0:
        benchmark_2 = step_benchmark_1(benchmark_2)
        nb_todo = nb_todo_in_benchmark(benchmark_2)
    return benchmark_2
