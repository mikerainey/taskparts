import jsonschema
import simplejson as json

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
    ps=r['benchmark_run']
    rargs=string_of_cl_args(ps['cl_args'])
    env_args=string_of_env_args(ps['env_args'])
    return env_args + ' ' + ps['path'] + ' ' + rargs
    

with open('benchmark_run_series_schema.json', 'r') as f:
    schema_data = f.read()
    schema = json.loads(schema_data)

with open('benchmark_run_series_ex1.json', 'r') as f:
    series1 = json.load(f)
    jsonschema.validate(series1, schema)
    str = string_of_benchmark_run(series1['runs'][0])
    print(str)
