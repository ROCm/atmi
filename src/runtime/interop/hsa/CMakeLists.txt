# ===--------------------------------------------------------------------------
#               ATMI (Asynchronous Task and Memory Interface)
#
#  This file is distributed under the MIT License. See LICENSE.txt for details.
# ===--------------------------------------------------------------------------

libatmi_runtime_say("Preparing to build runtime/interop/hsa")
target_sources(atmi_runtime PUBLIC
    ${PROJECT_SOURCE_DIR}/runtime/interop/hsa/atmi_interop_hsa.cpp 
    )

include_directories(${ROCM_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../../include)

# set output dir for .h files
set (OUTPUT_INC_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../../include)

# create output dir for include files
execute_process(COMMAND "/bin/mkdir" "-p" "${OUTPUT_INC_DIRECTORY}")

add_custom_command(
  OUTPUT ${OUTPUT_INC_DIRECTORY}/atmi_interop_hsa.h
  COMMAND /usr/bin/rsync ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include/atmi_interop_hsa.h ${OUTPUT_INC_DIRECTORY}/atmi_interop_hsa.h
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include/*.h
  )

add_custom_target(interop_header ALL DEPENDS ${OUTPUT_INC_DIRECTORY}/atmi_interop_hsa.h)

INSTALL(FILES 
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include/atmi_interop_hsa.h
        DESTINATION "include"
        COMPONENT runtime
       )

