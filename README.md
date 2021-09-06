# taskparts
Task-parallel runtime system: A C++ library to support task parallelism on multicore platforms

clang++ -pthread -std=c++17 -DTASKPARTS_POSIX -I../include test_scheduler.cpp
g++ -pthread -DTASKPARTS_POSIX -std=c++17 -g -I ../include test_scheduler.cpp
