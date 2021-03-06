from parameter import *

taskparts_benchmark_num_repeat_key = 'TASKPARTS_BENCHMARK_NUM_REPEAT'
taskparts_benchmark_warmup_secs_key = 'TASKPARTS_BENCHMARK_WARMUP_SECS'
taskparts_outfile_key = 'TASKPARTS_STATS_OUTFILE'
taskparts_cilk_outfile_key = 'CILK_STATS_OUTFILE'
taskparts_num_workers_key = 'TASKPARTS_NUM_WORKERS'
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
taskparts_resource_binding_key = 'TASKPARTS_RESOURCE_BINDING'

taskparts_env_vars = [
    taskparts_num_workers_key,
    taskparts_outfile_key,
    taskparts_benchmark_num_repeat_key,
    taskparts_benchmark_warmup_secs_key,
    taskparts_cilk_outfile_key,
    taskparts_resource_binding_key
]

def mk_taskparts_num_repeat(n):
    return mk_parameter(taskparts_benchmark_num_repeat_key, n)

def mk_taskparts_num_workers(n):
    return mk_parameter(taskparts_num_workers_key, n)

def mk_taskparts_warmup_secs(n):
    return mk_parameter(taskparts_benchmark_warmup_secs_key, n)
