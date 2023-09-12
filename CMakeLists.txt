cmake_minimum_required(VERSION 3.14)
project(taskparts)
set(CMAKE_CXX_STANDARD 17)
add_compile_options(
	-fno-stack-protector
	-fno-asynchronous-unwind-tables
)

option(NONELASTIC "Disable elastic scheduling." OFF)
option(HWLOC "Use hwloc." OFF)
option(STATS "Enable collecting and reporting of stats on scheduling behavior." OFF)
option(LOGGING "Enable collecting and reporting of logging on scheduling behavior." OFF)
option(CHASELEV_DEQUE "Use the concurrent deque of Chase and Lev." OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

if (NONELASTIC)
    message(STATUS "Disabling elastic scheduling")
    add_compile_options(-DTASKPARTS_DISABLE_ELASTIC)
endif()
if (STATS)
    message(STATUS "Enabling stats.")
    add_compile_options(-DTASKPARTS_STATS)
endif()
if (LOGGING)
    message(STATUS "Enabling logging.")
    add_compile_options(-DTASKPARTS_LOGGING)
endif()
if (CHASELEV_DEQUE)
    if (NOT NONELASTIC)
       message(FATAL_ERROR "Cannot use Chase and Lev deque in combination with elastic scheduling.")
    endif()
    message(STATUS "Using Chase and Lev deque.")
    add_compile_options(-DTASKPARTS_USE_CHASELEV_DEQUE)
endif()


find_package(HWLOC REQUIRED)
if (HWLOC)
    message(STATUS "Finding hwloc header files.")
    include_directories(${HWLOC_INCLUDE_DIRS}/include)
    add_compile_options(-DTASKPARTS_USE_HWLOC)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(taskparts SHARED src/taskparts.cpp)

if (HWLOC)
    message(STATUS "Linking with hwloc.")
    target_link_libraries(${CMAKE_PROJECT_NAME} hwloc)
endif()

install(TARGETS taskparts DESTINATION lib)
install(FILES include/taskparts/taskparts.hpp include/taskparts/taskparts_internal.hpp DESTINATION include/taskparts)

