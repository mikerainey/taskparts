import subprocess, os, sys, socket, signal, threading, time, pathlib, psutil
import simplejson as json, jsonschema
from datetime import datetime

module_cwd = os.getcwd()

with open('benchmark_run_schema.json', 'r') as f:
    benchmark_run_schema = json.loads(f.read())

def validate_benchmark_run(b):
    # We change the working directory here because the benchmark
    # schema refers to the parameter schema, and the reference can be
    # resolved only in the folder containing the latter schema.
    original_path = os.getcwd()
    os.chdir(module_cwd)
    jsonschema.validate(b, benchmark_run_schema)
    os.chdir(original_path)

def read_benchmark_from_file_path(file_path):
    with open(file_path, 'r', encoding='utf-8') as fd:
        bench_2 = json.load(fd)
        fd.close()
        return bench_2

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

def string_of_cl_args(args):
    out = ''
    i = 0
    for a in args:
        sa = str(a)
        out = (out + ' ' + sa) if i > 0 else sa
        i = i + 1
    return out
    
def string_of_benchmark_run(r):
    br = r['benchmark_run']
    cl_args = (' ' if br['cl_args'] != [] else '') + string_of_cl_args(br['cl_args'])
    env_args = string_of_env_args(br['env_args']) + (' ' if br['env_args'] != [] else '')
    return env_args + br['path_to_executable'] + cl_args

class Nonzero_return_code(Exception):
    pass

# br_i: benchmark run formatted in in json
# cwd: working directory in which to run the benchmark binary (if not
# the None value)
# timeout_sec: a number of seconds to allow the benchmark to run,
# after which we kill the benchmark run (if not the None value)
def run_benchmark(br_i, cwd = None, timeout_sec = None):
    # check that the input describes a valid benchmark run
    validate_benchmark_run(br_i)
    # build the command string and create a corresponding subprocess, which is ready to run
    cmd = string_of_benchmark_run(br_i)    
    os_env = os.environ.copy()
    env_args = { a['var']: str(a['val']) for a in br_i['benchmark_run']['env_args'] }
    env = {**os_env, **env_args}
    current_child = subprocess.Popen(cmd, shell = True, env = env_args,
                                     stdout = subprocess.PIPE, stderr = subprocess.PIPE,
                                     cwd = cwd)
    # initialize output data
    ts = time.time()
    child_stdout = ''
    child_stderr = ''
    return_code = None
    benchmark_failed = False
    # start the benchmark running
    try:
        child_stdout, child_stderr = current_child.communicate(timeout = timeout_sec)
        child_stdout = child_stdout.decode('utf-8')
        child_stderr = child_stderr.decode('utf-8')
        return_code = current_child.returncode
        if return_code != 0:
            raise Nonzero_return_code
    except Nonzero_return_code:
        print('Warning: benchmark run returned error code ' + str(return_code))
        print('stdout:')
        print(str(child_stdout))
        print('stderr:')
        print(str(child_stderr))
        benchmark_failed = True
    except subprocess.TimeoutExpired:
        # Here, we kill the current_child process and all of its
        # children to ensure the call to communicate() returns
        # immediately.
        parent = psutil.Process(current_child.pid)
        for child in parent.children(recursive=True):
            child.kill()
        parent.kill()
        child_stdout, child_stderr = current_child.communicate()
        child_stdout = child_stdout.decode('utf-8')
        child_stderr = child_stderr.decode('utf-8')
        print('Warning: benchmark timed out after ' + str(timeout_sec) + ' seconds')
        print('stdout:')
        print(str(child_stdout))
        print('stderr:')
        print(str(child_stderr))
        # The return_code field is hereby left out of the trace,
        # indicating timeout.
        benchmark_failed = True
    # collect output and return the benchmark-run record, extended with the output
    br_o = br_i
    if return_code != None:
        br_o['benchmark_run']['return_code'] = return_code
    br_o['benchmark_run']['timestamp'] = datetime.now().strftime("%y-%m-%d %H:%M:%S.%f")
    br_o['benchmark_run']['host'] = socket.gethostname()
    br_o['benchmark_run']['elapsed'] = time.time() - ts
    br_o['benchmark_run']['stdout'] = str(child_stdout)
    br_o['benchmark_run']['stderr'] = str(child_stderr)
    return br_o
