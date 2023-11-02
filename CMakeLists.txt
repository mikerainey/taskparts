cmake_minimum_required(VERSION 3.14)
project(taskparts)
set(CMAKE_CXX_STANDARD 17)

option(TASKPARTS_HEADER_ONLY "Build as a header-file-only library." OFF)

if(TASKPARTS_HEADER_ONLY)
  message("Building as header-only library.")
  add_definitions(-DTASKPARTS_HEADER_ONLY)
  add_library(taskparts INTERFACE)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_include_directories(taskparts INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  install(DIRECTORY include/taskparts DESTINATION include)
  install(FILES src/taskparts.cpp DESTINATION src)
else()
  # Required compile-time flags
  add_compile_options(
	  -fno-stack-protector
	  -fno-asynchronous-unwind-tables
  )

  # TaskPaRTS build options
  option(NONELASTIC "Disable elastic scheduling." OFF)
  option(STATS "Enable collecting and reporting of stats on scheduling behavior." OFF)
  option(LOGGING "Enable collecting and reporting of logging on scheduling behavior." OFF)
  option(CHASELEV_DEQUE "Use the concurrent deque of Chase and Lev." OFF)
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

  # This command needs to be before the hwloc configuration
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

  # Optional dependency: hwloc
  find_package(HWLOC)
  if (HWLOC_FOUND)
      message(STATUS "Using hwloc.")
      include_directories(${HWLOC_INCLUDE_DIRS}/include)
      add_compile_options(-DTASKPARTS_USE_HWLOC)
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

