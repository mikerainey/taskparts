from benchmark import *

def pretty_print_json(j):
    print(json.dumps(j, indent=2))


taskparts_outfile_key = 'TASKPARTS_STATS_OUTFILE'
taskparts_num_workers_key = 'TASKPARTS_NUM_WORKERS'
q = mk_cross(mk_parameter('path_to_executable', '../bin/fib_nativeforkjoin.sta'),
             mk_parameters(taskparts_num_workers_key, [12,16]))

mods = modifiers = {
    'path_to_executable_key': 'path_to_executable',
    'outfile_keys': [taskparts_outfile_key],
    'env_vars': [taskparts_outfile_key, taskparts_num_workers_key]
}
bench = mk_benchmark(q, mods)
pretty_print_json(bench)
print('=====step==>')
pretty_print_json(step_benchmark(bench))

