from benchmark_run import *

for i in range(1, 4):
    print(run_benchmark(read_benchmark_from_file_path('benchmark_run_example_' + str(i) + '.json'),
                        timeout_sec = 5))
