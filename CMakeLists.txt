cmake_minimum_required(VERSION 3.14)
project(taskparts)
set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(taskparts SHARED src/taskparts.cpp)

install(TARGETS taskparts DESTINATION lib)
install(FILES include/taskparts/taskparts.hpp include/taskparts/taskparts_internal.hpp DESTINATION include/taskparts)
