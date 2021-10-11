import jsonschema, subprocess, tempfile, os, sys, socket, signal, threading, time
import simplejson as json
from datetime import datetime
from parameter import *

with open('benchmark_schema.json', 'r') as f:
    benchmark_schema = json.loads(f.read())

# Benchmark queries
# =================

def nb_todo_in_benchmark(b):
    return len(b['todo']['value'])

def nb_done_in_benchmark(b):
    return len(b['done']['value'])

def nb_traced_in_benchmark(b):
    return len(b['trace'])

# Benchmark construction
# ======================

def mk_benchmark_run(row,
                     path_to_executable_key = 'path_to_executable',
                     env_vars = [],
                     silent_keys = []):
    d = row_to_dictionary(row)
    p = d[path_to_executable_key]
    cl_args = [ {'var': kvp['key'], 'val': kvp['val']}
                for kvp in row
                if not(kvp['key'] in ([path_to_executable_key] + env_vars + silent_keys)) ]
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
    b = { 'parameters': parameters,
          'modifiers': modifiers,
          'todo': todo,
          'trace': [],
          'done': done,
          'done_trace_links': [] }
    jsonschema.validate(b, benchmark_schema)
    return b

def seed_benchmark(benchmark_1):
    benchmark_2 = benchmark_1.copy()
    assert(get_first_key_in_dictionary(benchmark_2['todo']) == 'value')
    if nb_todo_in_benchmark(benchmark_2) == 0:
        benchmark_2['todo'] = eval(benchmark_1['parameters'])
    return benchmark_2

def serial_merge_benchmarks(b1, b2):
    bm = b2.copy()
    return bm

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

def string_of_benchmark_run(r,
                            show_env_args = False,
                            show_silent_args = False,
                            nb_traced = -1):
    br = r['benchmark_run']
    cl_args = (' ' if br['cl_args'] != [] else '') + string_of_cl_args(br['cl_args'])
    env_args = ''
    if show_env_args:
        env_args = string_of_env_args(br['env_args']) + (' ' if br['env_args'] != [] else '')
    silent_args = ''
    if show_silent_args:
        if 'silent_keys' in br:
            silent_args = 'silent:('
            for s in br['silent_keys']:
                silent_args += s + ' '
            silent_args += ')'
    return env_args + br['path_to_executable'] + cl_args + silent_args

def string_of_benchmark_progress(row_number, nb_traced, nb_todo):
    return '[' + str(row_number) + '/' + str(nb_traced + nb_todo) + '] $ '

def string_of_benchmark_runs(b, show_run_numbers = True):
    sr = ''
    if 'cwd' in b['modifiers']:
        sr += 'cd ' + b['modifiers']['cwd'] + '\n'
    b = seed_benchmark(b.copy())
    nb_traced = nb_traced_in_benchmark(b)
    nb_todo = nb_todo_in_benchmark(b)
    todo = b['todo']
    row_number = nb_traced + 1
    for next_row in b['todo']['value']:
        modifiers = b['modifiers'] 
        env_vars = modifiers['env_vars'] if 'env_vars' in modifiers else []
        silent_keys = modifiers['silent_keys'] if 'silent_keys' in modifiers else []
        next_run = mk_benchmark_run(next_row, modifiers['path_to_executable_key'], env_vars, silent_keys)
        pre = string_of_benchmark_progress(row_number, nb_traced, nb_todo) if show_run_numbers else ''
        sr += pre + string_of_benchmark_run(next_run, show_env_args = True, show_silent_args = True) + '\n'
        row_number += 1
    return sr

# Benchmark stepper
# =================

_currentChild = None
def _signalHandler(signal, frame):
  sys.stderr.write("[ERR] Interrupted.\n")
  if _currentChild:
    _currentChild.kill()
  sys.exit(1)
signal.signal(signal.SIGINT, _signalHandler)

def _killer():
  if _currentChild:
    try:
      _currentChild.kill()
    except Exception as e:
      sys.stderr.write("[WARN] Error while trying to kill process {}: {}\n".format(_currentChild.pid, str(e)))

def run_benchmark(cmd, env_args, cwd = ''):
    if cwd == '':
        return subprocess.Popen(cmd, shell = True, env = env_args,
                                stdout = subprocess.PIPE, stderr = subprocess.PIPE)
    else:
        return subprocess.Popen(cmd, shell = True, env = env_args,
                                stdout = subprocess.PIPE, stderr = subprocess.PIPE, cwd = cwd)

def step_benchmark_todo(todo_1, outfiles = []):
    next_todo_position = 0
    next_row = eval(mk_cross(todo_1, mk_outfile_keys(outfiles)))['value'].pop(next_todo_position)
    todo_2 = todo_1['value'].copy()
    todo_2.pop(next_todo_position)
    return next_row, todo_2

def create_benchmark_run_outfiles(outfile_keys):
    outfiles = []
    for k in outfile_keys:
        fd, n = tempfile.mkstemp(suffix = '.json', text = True)
        outfiles += [{'key': k, 'file_name': n, 'fd': fd}]
        os.close(fd)
    return outfiles

def collect_benchmark_run_outfiles(outfiles, next_row,
                                   client_format_to_row = lambda d: dictionary_to_row(d),
                                   parse_output_to_json = lambda x: x):
    results_expr = {'value': [ next_row ]}
    for of in outfiles:
        j = json.load(parse_output_to_json(open(of['file_name'], 'r')))
        open(of['file_name'], 'w').close()
        os.unlink(of['file_name'])
        results_expr = mk_cross(results_expr, {'value': [client_format_to_row(d) for d in j] })
    return results_expr

def step_benchmark_run(benchmark_1,
                       verbose = True,
                       client_format_to_row = lambda d: dictionary_to_row(d),
                       parse_output_to_json = lambda x: x,
                       timeout_sec = 0.0):
    jsonschema.validate(benchmark_1, benchmark_schema)
    nb_done = nb_done_in_benchmark(benchmark_1)
    nb_traced = nb_traced_in_benchmark(benchmark_1)
    nb_todo = nb_todo_in_benchmark(benchmark_1)
    parameters = benchmark_1['parameters']
    modifiers = benchmark_1['modifiers']
    # Try to load the next run to do
    benchmark_2 = seed_benchmark(benchmark_1)
    if nb_todo == 0:
        jsonschema.validate(benchmark_2, benchmark_schema)
        return benchmark_2
    # Create any tempfiles needed to collect results
    outfiles = create_benchmark_run_outfiles(modifiers['outfile_keys'])
    # Fetch the next run
    next_row, todo_2 = step_benchmark_todo(benchmark_2['todo'], outfiles)
    env_vars = modifiers['env_vars'] if 'env_vars' in modifiers else []
    silent_keys = modifiers['silent_keys'] if 'silent_keys' in modifiers else []
    next_run = mk_benchmark_run(next_row, modifiers['path_to_executable_key'], env_vars, silent_keys)
    # Print the command to be issued to the command line for the next run
    if verbose:
        print(string_of_benchmark_progress(nb_done + 1, nb_traced, nb_todo) +
              string_of_benchmark_run(next_run, show_env_args = True))
    # Do the next run
    current_child = run_benchmark(string_of_benchmark_run(next_run),
                                  { a['var']: str(a['val']) for a in next_run['benchmark_run']['env_args'] },
                                  modifiers['cwd'] if 'cwd' in modifiers else '')
    timer = threading.Timer(timeout_sec, _killer) if timeout_sec > 0.0 else -1
    ts = time.time()
    child_stdout = ''
    child_stderr = ''
    try:
        if timer != -1:
            timer.start()
        child_stdout, child_stderr = current_child.communicate()
        child_stdout = child_stdout.decode('utf-8')
        child_stderr = child_stderr.decode('utf-8')
        return_code = current_child.returncode
        if return_code != 0:
            print('Warning: benchmark run returned error code ' + str(return_code))
            print('stdout:')
            print(child_stdout)
            print('stderr:')
            print(child_stderr)
        # Collect the results
        results_expr = collect_benchmark_run_outfiles(outfiles, next_row,
                                                      client_format_to_row,
                                                      parse_output_to_json)
        benchmark_2['todo'] = { 'value': todo_2 }
        benchmark_2['done'] = eval(mk_append(benchmark_1['done'], results_expr))
    finally:
        if timer != -1:
            timer.cancel()
        return_code = 1
    # Save the results
    trace_2 = next_run.copy()
    trace_2['benchmark_run']['return_code'] = return_code
    trace_2['benchmark_run']['timestamp'] = datetime.now().strftime("%y-%m-%d %H:%M:%S.%f")
    trace_2['benchmark_run']['host'] = socket.gethostname()
    trace_2['benchmark_run']['elapsed'] = time.time() - ts
    trace_2['benchmark_run']['stdout'] = str(child_stdout)
    trace_2['benchmark_run']['stderr'] = str(child_stderr)
    benchmark_2['trace'] += [ trace_2 ]
    benchmark_2['done_trace_links'] += [ {'trace_position': nb_traced, 'done_position': nb_done } ]
    jsonschema.validate(benchmark_2, benchmark_schema)
    return benchmark_2

def step_benchmark(benchmark_1):
    benchmark_2 = seed_benchmark(benchmark_1)
    nb_todo = nb_todo_in_benchmark(benchmark_2)
    while nb_todo > 0:
        benchmark_2 = step_benchmark_run(benchmark_2)
        nb_todo = nb_todo_in_benchmark(benchmark_2)
    return benchmark_2

    
