# ===--------------------------------------------------------------------------
#               ATMI (Asynchronous Task and Memory Interface)
#
#  This file is distributed under the MIT License. See LICENSE.txt for details.
# ===--------------------------------------------------------------------------

# Enable support for C++11.
add_definitions(-std=c++11)

# If building this library in debug mode, we define a macro to enable
# dumping progress messages at runtime.
string( TOLOWER "${CMAKE_BUILD_TYPE}" ATMI_CMAKE_BUILD_TYPE)
if(ATMI_CMAKE_BUILD_TYPE MATCHES debug)
  add_definitions(-DDEBUG)
  add_definitions(-g)
  add_definitions(-O0)
else()
  add_definitions(-g)
  add_definitions(-O2)
endif()
add_definitions(-c)
add_definitions(-fpic)

# set output dir for the .so file
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../lib)

add_library(atmi_runtime SHARED
    atl_bindthread.c 
    atl_cputask.c    
    atl_profile.c    
    atl_rt.c         
    atl_synckernel.c 
    ATLData.cpp      
    ATLMachine.cpp   
    ATLQueue.cpp
    )

include_directories(${ATMI_DEP_LIBHSA_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../include)

## CPack component info
set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "ATMI Runtime Library")
set(CPACK_COMPONENT_RUNTIME_REQUIRED TRUE )

target_link_libraries(
  atmi_runtime
  "-Wl,--as-needed"
  -L${ATMI_DEP_LIBHSA_LIBRARIES_DIRS} -lhsa-runtime64
  )

# set output dir for .h files
set (OUTPUT_INC_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../include)

# create output dir for include files
execute_process(COMMAND "/bin/mkdir" "-p" "${OUTPUT_INC_DIRECTORY}")

add_custom_command(
  OUTPUT ${OUTPUT_INC_DIRECTORY}/atmi.h
  COMMAND /usr/bin/rsync ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/atmi.h ${OUTPUT_INC_DIRECTORY}/atmi.h
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/*.h
  )

add_custom_command(
  OUTPUT ${OUTPUT_INC_DIRECTORY}/atmi_runtime.h
  COMMAND /usr/bin/rsync ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/atmi_runtime.h ${OUTPUT_INC_DIRECTORY}/atmi_runtime.h
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/*.h
  )

add_custom_target(core_header ALL DEPENDS ${OUTPUT_INC_DIRECTORY}/atmi.h ${OUTPUT_INC_DIRECTORY}/atmi_runtime.h)

# Install plugin under the lib destination folder.
if(CMAKE_BUILD_TYPE MATCHES Debug)
  install(TARGETS atmi_runtime LIBRARY DESTINATION "lib-debug" COMPONENT runtime )
else()
  install(TARGETS atmi_runtime LIBRARY DESTINATION "lib" COMPONENT runtime )
endif()

INSTALL(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/atmi.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/atmi_runtime.h
        DESTINATION "include"
        COMPONENT runtime
       )

if(NOT ATMI_WITH_HCC2_VAR)
  INSTALL(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../bin/cloc.sh
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../bin/mymcpu
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../bin/mygpu
        PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        DESTINATION "bin"
        COMPONENT runtime
       )
endif()
