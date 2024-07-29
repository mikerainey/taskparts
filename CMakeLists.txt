cmake_minimum_required(VERSION 3.14)
project(taskparts)
set(CMAKE_CXX_STANDARD 17)

# Build options
option(SHARED_LINKED_LIBRARY "Build as a shared linked library." OFF)
option(NONELASTIC "Disable elastic scheduling." OFF)
option(STATS "Enable collecting and reporting of stats on scheduling behavior." OFF)
option(LOGGING "Enable collecting and reporting of logging on scheduling behavior." OFF)
option(CHASELEV_DEQUE "Use the concurrent deque of Chase and Lev." OFF)
option(HWLOC "Build with hwloc, which enables CPU pinning." OFF)

###################
# Header-only build
###################

if(NOT SHARED_LINKED_LIBRARY)
  message("Building as header-only library.")
  add_library(taskparts INTERFACE)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_compile_definitions(taskparts INTERFACE TASKPARTS_HEADER_ONLY)

  if (NONELASTIC)
      message(STATUS "Disabling elastic scheduling")
      target_compile_definitions(taskparts INTERFACE TASKPARTS_DISABLE_ELASTIC)
  endif()
  if (STATS)
      message(STATUS "Enabling stats.")
      target_compile_definitions(taskparts INTERFACE TASKPARTS_STATS)
  endif()
  if (LOGGING)
      message(STATUS "Enabling logging.")
      target_compile_definitions(taskparts INTERFACE TASKPARTS_LOGGING)
  endif()
  if (CHASELEV_DEQUE)
      if (NOT NONELASTIC)
	 message(FATAL_ERROR "Cannot use Chase and Lev deque in combination with elastic scheduling.")
      endif()
      message(STATUS "Using Chase and Lev deque.")
      target_compile_definitions(taskparts INTERFACE TASKPARTS_USE_CHASELEV_DEQUE)
  endif()

  if (HWLOC)
      find_package(HWLOC)
      if (HWLOC_FOUND)
	  message(STATUS "Using hwloc.")
	  target_link_libraries(taskparts INTERFACE hwloc::hwloc)
	  target_compile_definitions(taskparts INTERFACE TASKPARTS_USE_HWLOC)
      endif()
  endif()

  install(DIRECTORY include/taskparts DESTINATION include)
  install(FILES src/taskparts.cpp DESTINATION src)
  install(TARGETS taskparts EXPORT taskpartsConfig INCLUDES DESTINATION include src)
  install(EXPORT taskpartsConfig FILE taskpartsConfig.cmake NAMESPACE taskparts:: DESTINATION lib/cmake/taskparts)

######################
# Linked-library build
######################

else()
  # Required compile-time flags
  add_compile_options(
	  -fno-stack-protector
	  -fno-asynchronous-unwind-tables
  )

  # This command needs to be before the hwloc configuration
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

  if (HWLOC)
      find_package(HWLOC)
      if (HWLOC_FOUND)
	  message(STATUS "Using hwloc.")
	  include_directories(${HWLOC_INCLUDE_DIRS}/include)
	  add_compile_options(-DTASKPARTS_USE_HWLOC)
      endif()
  endif()

  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
  add_library(taskparts SHARED src/taskparts.cpp)

  if (HWLOC_FOUND)
      message(STATUS "Linking with hwloc.")
      target_link_libraries(${CMAKE_PROJECT_NAME} hwloc)
  endif()

  install(TARGETS taskparts DESTINATION lib)
  install(DIRECTORY include/taskparts DESTINATION include)
endif()		  

