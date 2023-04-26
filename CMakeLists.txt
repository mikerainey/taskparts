cmake_minimum_required(VERSION 3.14)
project (TaskPaRTS)
set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(TaskPaRTS SHARED src/taskparts.cpp)

install(TARGETS TaskPaRTS DESTINATION lib)
install(FILES include/taskparts/taskparts.hpp include/taskparts/taskparts_internal.hpp DESTINATION include/taskparts)
