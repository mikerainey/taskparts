import jsonschema
import simplejson as json
import subprocess
import tempfile
import os

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

def mk_nil():
    return []

def mk_parameter(n, v):
    return [{ n: v }]

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

def mapping_of_env_args(eas):
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

def select_rows_from_match_rows(rows, match_rows):
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

def select_all_matching_col_key(rows, col_key):
    result = []
    for row in rows:
        if col_key in row:
            result += [row[col_key]]
    return result         

def generate_chart(rows, x_axis_rows, y_col_key):
    xy_pairs = []
    for x_ar in x_axis_rows:
        rs = select_rows_from_match_rows(rows, [x_ar])
        x_v = list(x_ar.values())[0]
        y_vs = select_all_matching_col_key(rs, y_col_key)
        # todo: merge vs via client supplied function
        y_v = y_vs[0]
        xy_pairs += [{ "x": x_v, "y": y_v }]
    return {
        "chart": {
            "xy_pairs": xy_pairs
        }
    }

with open('benchmark_run_series_schema.json', 'r') as f:
    benchmark_run_series_schema = json.loads(f.read())

results_file_name = 'results.json'
trace_file_name = 'trace.json'
            
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
        ea_mp = mapping_of_env_args(env_args)
        print(string_of_env_args(env_args) + ' ' + string_of_benchmark_run(r))
        current_child = subprocess.Popen(cmd, shell = True, env = ea_mp,
                                         stdout = subprocess.PIPE, stderr = subprocess.PIPE)
        so, se = current_child.communicate()
        # print(so.decode("utf-8"))
        rc = current_child.returncode
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
        

rows = mk_cross(mk_append(mk_path_to_executable('./fib_oracleguided.sta'),
                          mk_path_to_executable('./fib_nativeforkjoin.sta')),
                mk_append(mk_parameter('TASKPARTS_NUM_WORKERS', '4'),
                          mk_parameter('TASKPARTS_NUM_WORKERS', '16')))


#run_benchmarks(rows = rows, env_var_names = ['TASKPARTS_NUM_WORKERS'])

with open(results_file_name, 'r') as f:
    rows = json.load(f)
    # print(rows)
    # print('---')
    result_rows = select_rows_from_match_rows(rows, mk_path_to_executable('./fib_nativeforkjoin.sta'))
    # print(result_rows)
    exectimes = select_all_matching_col_key(result_rows, 'exectime')
    print(generate_chart(rows, mk_append(mk_parameter('TASKPARTS_NUM_WORKERS', '4'),
                                         mk_parameter('TASKPARTS_NUM_WORKERS', '16')), 'exectime'))
    f.close()
