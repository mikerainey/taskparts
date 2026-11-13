# A CMake package named taskparts, which is the instrumented micro taskparts
# underneath, for ParlayLib's option PARLAY_TASKPARTS. Give the micro
# taskparts directory as MTP_DIR:
#
#   cmake -S parlaylib -B build -DPARLAY_EXAMPLES=On -DPARLAY_TASKPARTS=On \
#     -Dtaskparts_DIR=<this directory> -DMTP_DIR=<../microtaskparts>

if(NOT MTP_DIR)
  message(FATAL_ERROR "Set MTP_DIR to a micro taskparts directory")
endif()
if(NOT EXISTS "${MTP_DIR}/microtaskparts.hpp")
  message(FATAL_ERROR "MTP_DIR=${MTP_DIR} has no microtaskparts.hpp")
endif()

if(NOT TARGET taskparts::taskparts)
  add_library(taskparts::taskparts INTERFACE IMPORTED)
  set_target_properties(taskparts::taskparts PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR};${MTP_DIR}")
endif()
set(taskparts_FOUND TRUE)
