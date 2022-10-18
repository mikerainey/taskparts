import os, tempfile
from copy import deepcopy
from benchmark_run import *

# Inputs
# ======

# Environment variables
# ---------------------

taskparts_benchmark_num_repeat_key = 'TASKPARTS_BENCHMARK_NUM_REPEAT'
taskparts_benchmark_warmup_secs_key = 'TASKPARTS_BENCHMARK_WARMUP_SECS'
taskparts_outfile_key = 'TASKPARTS_STATS_OUTFILE'
taskparts_cilk_outfile_key = 'CILK_STATS_OUTFILE'
taskparts_num_workers_key = 'TASKPARTS_NUM_WORKERS'
taskparts_resource_binding_key = 'TASKPARTS_RESOURCE_BINDING'
taskparts_nb_steal_attempts_key = 'TASKPARTS_NB_STEAL_ATTEMPTS'

taskparts_env_vars = [
    taskparts_num_workers_key,
    taskparts_outfile_key,
    taskparts_benchmark_num_repeat_key,
    taskparts_benchmark_warmup_secs_key,
    taskparts_cilk_outfile_key,
    taskparts_resource_binding_key
]

# Outputs
# =======

# Stats
# -----

taskparts_exectime_key = 'exectime'
taskparts_usertime_key = 'usertime'
taskparts_systime_key = 'systime'
taskparts_maxrss_key = 'maxrss'
taskparts_total_time_key = 'total_time'
taskparts_total_work_time_key = 'total_work_time'
taskparts_total_idle_time_key = 'total_idle_time'
taskparts_total_sleep_time_key = 'total_sleep_time'
taskparts_utilization_key = 'utilization'
taskparts_nb_fibers_key = 'nb_fibers'
taskparts_nb_steals_key = 'nb_steals'

# Benchmark-run driver
# ====================

def run_taskparts_benchmark(br, num_repeat = None, warmup_secs = None,
                            cwd = None, timeout_sec = None, verbose = False):
    br_i = deepcopy(br)
    # generate a temporary file in which to store the stats output
    stats_fd, stats_path = tempfile.mkstemp(suffix = '.json', text = True)
    os.close(stats_fd)
    # let the taskparts runtime know about the temporary file above
    br_i['benchmark_run']['env_args'] += [{'var': taskparts_outfile_key, 'val': stats_path}]
    # set up other taskparts parameters
    if num_repeat != None:
        br_i['benchmark_run']['env_args'] += [{'var': taskparts_benchmark_num_repeat_key, 'val': num_repeat}]
    if warmup_secs != None:
        br_i['benchmark_run']['env_args'] += [{'var': taskparts_benchmark_warmup_secs_key, 'val': warmup_secs}]
    if verbose:
        print(string_of_benchmark_run(br))
    # run the benchmark
    br_o = run_benchmark(br_i, cwd, timeout_sec)
    # collect the stats output of the benchmark run
    stats = []
    if os.stat(stats_path).st_size != 0:
        stats = json.load(open(stats_path, 'r'))
    # remove the temporary file we used for the stats output
    open(stats_path, 'w').close()
    os.unlink(stats_path)
    return {'benchmark_results': br_o, 'stats': stats }

