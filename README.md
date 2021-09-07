# taskparts
Task-parallel runtime system: A C++ library to support task parallelism on multicore platforms

clang++ -std=c++17 -pthread -DTASKPARTS_POSIX -DTASKPARTS_X64 -I../include test_scheduler.cpp
g++ -std=c++17 -pthread -DTASKPARTS_POSIX -DTASKPARTS_X64 -g -I ../include test_scheduler.cpp

./gen_rollforward.sh sum_array sa
clang++ -std=c++17 -pthread -DTASKPARTS_POSIX -DTASKPARTS_X64 -I../include sum_array_manual.s sum_array.cpp
