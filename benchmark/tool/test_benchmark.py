from benchmark import *

def pretty_print_json(j):
    print(json.dumps(j, indent=4))

# p1 = mk_parameter(path_to_executable, 'bar')
# p2 = mk_parameter('foo2', 321.0)
# p3 = mk_parameter('baz', 555)
# p4 = mk_parameters('baz', [1,2,3])
# p = mk_cross(p3, p3)
# q = mk_cross(p1, mk_append(p2, p3))
#r0 = eval(q)
#print(r0)
#b=mk_benchmark_runs(r0, ['baz'])
#print(b)
#print(string_of_benchmark_run(b))
#def mk_benchmark_runs(params, env_vars):
#print(string_of_dry_runs(q, env_vars=['foo2'], outfile_keys = ['statsf']))
taskparts_outfile_key = 'TASKPARTS_STATS_OUTFILE'
taskparts_num_workers_key = 'TASKPARTS_NUM_WORKERS'
q = mk_cross(mk_parameter(path_to_executable, '../bin/fib_nativeforkjoin.sta'),
             mk_parameters(taskparts_num_workers_key, [12,16]))
print(string_of_dry_runs(q,
                         env_vars=[taskparts_num_workers_key, taskparts_outfile_key],
                         outfile_keys = [taskparts_outfile_key]))
do_benchmark_runs(q,
                  env_vars=[taskparts_num_workers_key, taskparts_outfile_key],
                  outfile_keys = [taskparts_outfile_key])

