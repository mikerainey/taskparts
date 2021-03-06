import jsonschema, subprocess, tempfile, os, sys, socket, signal, threading, time, hashlib, pathlib, psutil
import simplejson as json
from datetime import datetime
from parameter import *

# JSON schema validation for benchmark records
# ============================================

module_cwd = os.getcwd()

with open('benchmark_schema.json', 'r') as f:
    benchmark_schema = json.loads(f.read())

def validate_benchmark(b):
    # We change the working directory because the benchmark schema
    # refers to the parameter schema, and the reference can be
    # resolved only in the folder containing the latter schema.
    original_path = os.getcwd()
    os.chdir(module_cwd)
    jsonschema.validate(b, benchmark_schema)
    os.chdir(original_path)

# Benchmark queries
# =================

def nb_todo_in_benchmark(b):
    return len(b['todo']['value'])

def nb_done_in_benchmark(b):
    return len(b['done']['value'])

def nb_failed_in_benchmark(b):
    return len(b['failed']['value'])

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
    return mk_cross_sequence([mk_parameter(r['key'], r['file_path']) for r in outfiles])

def mk_benchmark(parameters,
                 modifiers = {
                     'path_to_executable_key': 'path_to_executable',
                     'outfile_keys': []
                 },
                 todo = mk_nil(), trace = [], done = mk_nil(), failed = mk_nil()):
    b = { 'parameters': parameters,
          'modifiers': modifiers,
          'todo': todo,
          'trace': [],
          'done': done,
          'done_trace_links': [],
          'failed': failed,
          'failed_trace_links': []
         }
    validate_benchmark(b)
    return b

def seed_benchmark(benchmark_1):
    benchmark_2 = benchmark_1.copy()
    assert(get_first_key_in_dictionary(benchmark_2['todo']) == 'value')
    if nb_todo_in_benchmark(benchmark_2) == 0:
        benchmark_2['todo'] = eval(benchmark_1['parameters'])
    return benchmark_2

# String conversions & hashing
# ============================

def string_of_benchmark_progress(row_number, nb_todo):
    return '[' + str(row_number) + '/' + str(nb_todo) + '] $ '

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
                            show_silent_args = False):
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

def string_of_benchmark_runs(b, show_run_numbers = True):
    sr = ''
    if 'cwd' in b['modifiers']:
        sr += '$ cd ' + b['modifiers']['cwd'] + ' # directory change as requested\n'
    b = seed_benchmark(b.copy())
    nb_todo = nb_todo_in_benchmark(b)
    nb_done_this_batch = 0
    todo = b['todo']
    for next_row in b['todo']['value']:
        modifiers = b['modifiers'] 
        env_vars = modifiers['env_vars'] if 'env_vars' in modifiers else []
        silent_keys = modifiers['silent_keys'] if 'silent_keys' in modifiers else []
        next_run = mk_benchmark_run(next_row, modifiers['path_to_executable_key'], env_vars, silent_keys)
        pre = string_of_benchmark_progress(nb_done_this_batch + 1, nb_todo) if show_run_numbers else ''
        sr += pre + string_of_benchmark_run(next_run, show_env_args = True, show_silent_args = True)
        if nb_done_this_batch + 1 != nb_todo:
            sr += '\n'
        nb_done_this_batch += 1
    return sr

def json_dumps(thing):
    return json.dumps(
        thing,
        ensure_ascii=False,
        sort_keys=True,
        indent=2,
        separators=(',', ':'),
    )

def get_hash(thing):
    return hashlib.sha256(json_dumps(thing).encode('utf-8')).hexdigest()

# Benchmark stepper
# =================

def run_benchmark(cmd, env_args, cwd = None, timeout = None):
    # later: make it an option whether or not to run the benchmark w/ the calling environment
    os_env = os.environ.copy()
    env = {**os_env, **env_args}
    return subprocess.Popen(cmd, shell = True, env = env,
                            stdout = subprocess.PIPE, stderr = subprocess.PIPE,
                            cwd = cwd)

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
        outfiles += [{'key': k, 'file_path': n, 'fd': fd}]
        os.close(fd)
    return outfiles

def collect_benchmark_run_outfiles(outfiles,
                                   next_row,
                                   client_format_to_row = lambda d: dictionary_to_row(d),
                                   parse_output_to_json = lambda x: x):
    results_expr = {'value': [ next_row ]}
    for of in outfiles:
        file_path = of['file_path']
        if os.stat(file_path).st_size == 0:
            continue
        j = json.load(parse_output_to_json(open(file_path, 'r')))
        open(file_path, 'w').close()
        os.unlink(file_path)
        results_expr = mk_cross(results_expr, {'value': [client_format_to_row(d) for d in j] })
    return results_expr

# later: make it possible to capture output of benchmark via stdout?

class Nonzero_return_code(Exception):
    pass

def step_benchmark_run(benchmark_1,
                       nb_done_this_batch = 0,
                       nb_todo_this_batch = 0,
                       done_peek_keys = [],
                       verbose = True,
                       client_format_to_row = lambda d: dictionary_to_row(d),
                       parse_output_to_json = lambda x: x,
                       timeout = None):
    validate_benchmark(benchmark_1)
    nb_done = nb_done_in_benchmark(benchmark_1)
    nb_traced = nb_traced_in_benchmark(benchmark_1)
    nb_todo = nb_todo_in_benchmark(benchmark_1)
    nb_failed = nb_failed_in_benchmark(benchmark_1)
    parameters = benchmark_1['parameters']
    modifiers = benchmark_1['modifiers']
    # Try to load the next run to do
    benchmark_2 = seed_benchmark(benchmark_1)
    if nb_todo == 0:
        validate_benchmark(benchmark_2)
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
        print(string_of_benchmark_progress(nb_done_this_batch + 1, nb_todo_this_batch) +
              string_of_benchmark_run(next_run, show_env_args = True))
    # Do the next run
    current_child = run_benchmark(string_of_benchmark_run(next_run),
                                  { a['var']: str(a['val']) for a in next_run['benchmark_run']['env_args'] },
                                  modifiers['cwd'] if 'cwd' in modifiers else None)
    ts = time.time()
    child_stdout = ''
    child_stderr = ''
    return_code = None
    benchmark_failed = False
    try:
        child_stdout, child_stderr = current_child.communicate(timeout = timeout)
        child_stdout = child_stdout.decode('utf-8')
        child_stderr = child_stderr.decode('utf-8')
        return_code = current_child.returncode
        if return_code != 0:
            raise Nonzero_return_code
        # Collect the results
        results_expr = collect_benchmark_run_outfiles(outfiles,
                                                      next_row,
                                                      client_format_to_row,
                                                      parse_output_to_json)
        benchmark_2['done'] = eval(mk_append(benchmark_1['done'], results_expr))
        benchmark_2['done_trace_links'] += [ {'trace_position': nb_traced, 'done_position': nb_done } ]
        if verbose:
            s = ''
            d = row_to_dictionary(benchmark_2['done']['value'][-1])
            for pk in done_peek_keys:
                if pk in d:
                    s += str(pk) + ' = ' + str(d[pk]) + ' '
            if s != '':
                s += '\n'
            print(s, end = '')
    except Nonzero_return_code:
        print('Warning: benchmark run returned error code ' + str(return_code))
        print('stdout:')
        print(str(child_stdout))
        print('stderr:')
        print(str(child_stderr))
        benchmark_failed = True
    except subprocess.TimeoutExpired:
        # Next, we kill the current_child process and all of its
        # children to ensure the call to communicate() returns
        # immediately.
        parent = psutil.Process(current_child.pid)
        for child in parent.children(recursive=True):
            child.kill()
        parent.kill()
        child_stdout, child_stderr = current_child.communicate()
        child_stdout = child_stdout.decode('utf-8')
        child_stderr = child_stderr.decode('utf-8')
        print('Warning: benchmark timed out after ' + str(timeout) + ' seconds')
        print('stdout:')
        print(str(child_stdout))
        print('stderr:')
        print(str(child_stderr))
        # The return_code field is hereby left out of the trace,
        # indicating timeout.
        benchmark_failed = True
    benchmark_2['todo'] = { 'value': todo_2 }
    # Save the results
    trace_2 = next_run.copy()
    if return_code != None:
        trace_2['benchmark_run']['return_code'] = return_code
    trace_2['benchmark_run']['timestamp'] = datetime.now().strftime("%y-%m-%d %H:%M:%S.%f")
    trace_2['benchmark_run']['host'] = socket.gethostname()
    trace_2['benchmark_run']['elapsed'] = time.time() - ts
    trace_2['benchmark_run']['stdout'] = str(child_stdout)
    trace_2['benchmark_run']['stderr'] = str(child_stderr)
    benchmark_2['trace'] += [ trace_2 ]
    if benchmark_failed:
        benchmark_2['failed'] = eval(mk_append(benchmark_1['failed'], {'value': [ next_row ]}))
        benchmark_2['failed_trace_links'] += [ {'trace_position': nb_traced, 'failed_position': nb_failed } ]
    validate_benchmark(benchmark_2)
    return benchmark_2

# later: save benchmark runs incrementally, based on some client specification
def step_benchmark(benchmark_1, done_peek_keys = [], timeout = None):
    benchmark_2 = seed_benchmark(benchmark_1)
    nb_done_this_batch = 0
    nb_todo = nb_todo_in_benchmark(benchmark_2)
    nb_todo_this_batch = nb_todo
    while nb_todo > 0:
        benchmark_2 = step_benchmark_run(benchmark_2,
                                         nb_done_this_batch = nb_done_this_batch,
                                         nb_todo_this_batch = nb_todo_this_batch,
                                         done_peek_keys = done_peek_keys,
                                         timeout = timeout)
        nb_todo = nb_todo_in_benchmark(benchmark_2)
        nb_done_this_batch += 1
    return benchmark_2

def serial_merge_benchmarks(b1, b2):
    validate_benchmark(b1)
    validate_benchmark(b2)
    b3 = b2.copy()
    b3['todo'] = eval(mk_append(b1['todo'], b2['todo']))
    b3['trace'] = b1['trace'] + b2['trace']
    b3['done'] = eval(mk_append(b1['done'], b2['done']))
    b3['failed'] = eval(mk_append(b1['failed'], b2['failed']))
    nb_done = nb_done_in_benchmark(b1)
    nb_failed = nb_failed_in_benchmark(b1)
    nb_traced = nb_traced_in_benchmark(b1)
    b2_done_trace_links = [ {'trace_position': dtl['trace_position'] + nb_traced,
                             'done_position': dtl['done_position'] + nb_done }
                             for dtl in b2['done_trace_links'] ]
    b3['done_trace_links'] = b1['done_trace_links'] + b2_done_trace_links
    b2_failed_trace_links = [ {'trace_position': dtl['trace_position'] + nb_traced,
                               'failed_position': dtl['failed_position'] + nb_failed }
                              for dtl in b2['failed_trace_links'] ]
    b3['failed_trace_links'] = b1['failed_trace_links'] + b2_failed_trace_links
    b3['parent_hashes'] = [ get_hash(json_dumps(b1)), get_hash(json_dumps(b2)) ]
    validate_benchmark(b3)
    return b3
    
# Benchmark I/O
# =============

def write_string_to_file_path(bstr, file_path = './results.json', verbose = True):
    with open(file_path, 'w', encoding='utf-8') as fd:
        fd.write(bstr)
        fd.close()
        if verbose:
            print('Wrote benchmark to file: ' + file_path)

def write_benchmark_to_file_path(benchmark, file_path = '', verbose = True):
    bstr = json_dumps(benchmark)
    file_path = file_path if file_path != '' else 'results-' + get_hash(bstr) + '.json'
    write_string_to_file_path(bstr, file_path, verbose)
    return file_path

def read_benchmark_from_file_path(file_path):
    with open(file_path, 'r', encoding='utf-8') as fd:
        bench_2 = json.load(fd)
        fd.close()
        return bench_2

def append_benchmark_to_file_path(benchmark_2, file_path_benchmark_1, file_path_benchmark_result = '',
                                  verbose = True):
    benchmark_1 = read_benchmark_from_file_path(file_path_benchmark_1)
    write_benchmark_to_file_path(benchmark_2)
    benchmark_3 = serial_merge_benchmarks(benchmark_1, benchmark_2)
    if verbose:
        print('Appending benchmarks: ' + get_hash(json_dumps(benchmark_1)) +
              ' and ' + get_hash(json_dumps(benchmark_2)) +
              ' to make ' + get_hash(json_dumps(benchmark_3)))
    return write_benchmark_to_file_path(benchmark_3, file_path_benchmark_result)

results_head_file_name = 'results-head.json'
default_results_folder_name = '_results'

def add_benchmark_to_results_repository(benchmark,
                                        results_repository_path = default_results_folder_name,
                                        verbose = True):
    pathlib.Path(results_repository_path).mkdir(parents=True, exist_ok=True)
    original_path = os.getcwd()
    os.chdir(results_repository_path)
    benchmark_file_path = ''
    if not(pathlib.Path(results_head_file_name).is_file()):
        benchmark_file_path = write_benchmark_to_file_path(benchmark, verbose = verbose)
    else:
        benchmark_file_path = append_benchmark_to_file_path(benchmark,
                                                            file_path_benchmark_1 = results_head_file_name,
                                                            verbose = verbose)
        os.unlink(results_head_file_name)
    pathlib.Path(results_head_file_name).symlink_to(pathlib.Path(benchmark_file_path))
    os.chdir(original_path)

def read_head_from_benchmark_repository(results_repository_path = default_results_folder_name):
    original_path = os.getcwd()
    os.chdir(results_repository_path)
    benchmark = read_benchmark_from_file_path(results_head_file_name)
    os.chdir(original_path)
    return benchmark
