# For reference:
#   https://github.com/justusc/FindTBB/blob/master/FindTBB.cmake

include(FindPackageHandleStandardArgs)

set(TASKPARTS_DEFAULT_SEARCH_DIR "/home/rainey/Work/successor/result")

find_path(TASKPARTS_INCLUDE_DIRS taskparts/taskparts.hpp
      HINTS ${TASKPARTS_INCLUDE_DIR} ${TASKPARTS_SEARCH_DIR}
      PATHS ${TASKPARTS_DEFAULT_SEARCH_DIR}
      PATH_SUFFIXES include)
      
message(STATUS "taskparts includes ${TASKPARTS_INCLUDE_DIRS}")

add_library(TASKPARTS::taskparts INTERFACE IMPORTED)
set_property(TARGET TASKPARTS::taskparts PROPERTY
          INTERFACE_INCLUDE_DIRECTORIES ${TASKPARTS_INCLUDE_DIRS})
set_property(TARGET TASKPARTS::taskparts PROPERTY
            INTERFACE_LINK_OPTIONS "-L${TASKPARTS_DEFAULT_SEARCH_DIR}/lib")
set_property(TARGET TASKPARTS::taskparts PROPERTY
          INTERFACE_LINK_LIBRARIES "taskparts")
set(TASKPARTS_FOUND 1)