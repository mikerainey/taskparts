from benchmark import *

p1 = mk_parameter(path_to_executable, 'bar')
p2 = mk_parameter('foo2', 321.0)
p3 = mk_parameter('baz', 555)
p4 = mk_parameters('baz', [1,2,3])
p = mk_cross(p3, p3)
q = mk_cross(p1, mk_append(p2, p3))
r0 = eval(q)
#print(r0)
b=mk_benchmark_runs(r0, ['baz'], [])
#print(b)
#print(string_of_benchmark_run(b))
#def mk_benchmark_runs(params, env_vars, silent_vars):
print(dry_runs(q, env_vars=['foo2']))
