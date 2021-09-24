import jsonschema
import simplejson as json
import subprocess

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

def mk_cl_arg(n, v):
    return [{ n: v }]

def mk_append(ps1, ps2):
    ps = ps1.copy()
    ps.extend(ps2)
    return ps

def mk_cross(ps1, ps2):
    ps = []
    for i1 in range(len(ps1)):
        for i2 in range(len(ps2)):
            r1 = ps1[i1]
            r2 = ps2[i2]
            r = r1
            for k in r2:
                r[k] = r2[k]
            ps.append(r)
    return ps

def flatten_dict(d):
    a = []
    for k in d.keys():
        a = a + [k, d[k]]
    return a

def mk_benchmark_run(p, env_var_names):
    pte = p['path_to_executable']
    clas = { key: value for key, value in p.items() if not (key in env_var_names) and key != 'path_to_executable' }
    envs = []
    for k in { key for key, value in p.items() if (key in env_var_names) }:
        envs = envs + [{ 'var': k, 'val': p[k] }]
    br = {
    "benchmark_run": {
        "path_to_executable": pte,
        "cl_args": flatten_dict(clas),
        "env_args": envs
    }
    }
    return br

def mk_benchmark_runs(ps, env_var_names):
    a = []
    for p in ps:
        a = a + [mk_benchmark_run(p, env_var_names)]
    return {"runs": a }
    
def mapping_of_env_args(eas):
    m = { }
    for a in eas:
        m[a['var']] = a['val']
    return m
    
with open('benchmark_run_series_schema.json', 'r') as f:
    benchmark_run_series_schema = json.loads(f.read())
        
def load_benchmark_run_series(fn):
    with open(fn, 'r') as f:
        s = json.load(f)
        jsonschema.validate(s, benchmark_run_series_schema)
        return s

rs1 = mk_append(mk_cross(mk_cl_arg("path_to_executable", "/bin/baszhd"),mk_cl_arg("BAR", "2")),
                mk_cross(mk_cl_arg("path_to_executable", "/bin/333"),mk_cl_arg("-bee", "21")))
rs2 = mk_cross(mk_cl_arg("-baz", "31"),mk_cl_arg("BOO", "22"))
# print(rs1)
# print(rs2)
rs3 = mk_cross(rs1, rs2)
#print(rs3)

print(mk_benchmark_runs(rs3, ['BAR','BOO']))
    
#series = load_benchmark_run_series('benchmark_run_series_ex1.json')
series = mk_benchmark_runs(rs3, ['BAR','BOO'])
for r in series['runs']:
    r = r.copy()
    cmd = string_of_benchmark_run(r)
    env_args = r['benchmark_run']['env_args']
    stats_fn = 'foo.json'
    env_args.extend(json.loads('[{ "var": "TASKPARTS_STATS_OUTFILE", "val": "' + stats_fn + '" }]'))    
    ea_mp = mapping_of_env_args(env_args)
    print(string_of_env_args(env_args) + ' ' + string_of_benchmark_run(r))
    current_child = subprocess.Popen(cmd, shell = True, env = ea_mp, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    so, se = current_child.communicate()
    rc = current_child.returncode
    print(r)
    print(rc)
    print(so.decode("utf-8"))
