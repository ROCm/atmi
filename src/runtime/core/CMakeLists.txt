# ===--------------------------------------------------------------------------
#               ATMI (Asynchronous Task and Memory Interface)
#
#  This file is distributed under the MIT License. See LICENSE.txt for details.
# ===--------------------------------------------------------------------------

# Enable support for C++11.
add_definitions(-std=c++11)

# Find libelf
find_package(LibElf REQUIRED)
include_directories(${LIBELF_INCLUDE_DIRS})
# libatmi_runtime_say("LIBELF: ${LIBELF_INCLUDE_DIRS} ${LIBELF_LIBRARIES} and ${LIBELF_FOUND}")

# Find comgr
find_package(amd_comgr REQUIRED)

set(amd_comgr_LIBRARIES amd_comgr)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "(ppc64le)|(aarch64)$")
   add_definitions(-DLITTLEENDIAN_CPU=1)
endif()

string( TOLOWER "${ATMI_DEVICE_RUNTIME}" ATMI_DEVICE_RUNTIME_VAR )
if(ATMI_DEVICE_RUNTIME_VAR MATCHES on )
  add_definitions(-DBUILD_WITH_DEVICE_RT)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../device_runtime)
endif()

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
add_definitions(-DROCM_VERSION_MAJOR=${ROCM_VERSION_MAJOR})
add_definitions(-DROCM_VERSION_MINOR=${ROCM_VERSION_MINOR})
add_definitions(-fpic)

# set output dir for the .so file
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../../lib)

#set soname version number
get_version ( "1.0.0" )

set ( BUILD_VERSION_MAJOR ${VERSION_MAJOR} )
set ( BUILD_VERSION_MINOR ${VERSION_MINOR} )
set ( BUILD_VERSION_PATCH ${VERSION_PATCH} )

set ( BUILD_VERSION_STRING "${BUILD_VERSION_MAJOR}.${BUILD_VERSION_MINOR}.${BUILD_VERSION_PATCH}" )

set ( LIB_VERSION_MAJOR 0 )
set ( LIB_VERSION_MINOR 7 )
if ( ${ROCM_PATCH_VERSION} )
    set ( LIB_VERSION_PATCH ${ROCM_PATCH_VERSION} )
else()
    set ( LIB_VERSION_PATCH 0 )
endif()
set ( LIB_VERSION_STRING "${LIB_VERSION_MAJOR}.${LIB_VERSION_MINOR}.${LIB_VERSION_PATCH}" )

add_library(atmi_runtime SHARED
    system.cpp
    cputask.cpp
    kernel.cpp
    task.cpp
    taskgroup.cpp
    data.cpp
    machine.cpp
    queue.cpp
    atmi.cpp
    utils.cpp
    )

set_property ( TARGET atmi_runtime PROPERTY VERSION "${LIB_VERSION_STRING}" )
set_property ( TARGET atmi_runtime PROPERTY SOVERSION "${LIB_VERSION_MAJOR}" )
set_property ( TARGET atmi_runtime PROPERTY LINK_FLAGS_RELEASE -s )

include_directories(${ROCM_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../include)

## CPack component info
set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "ATMI Runtime Library")
set(CPACK_COMPONENT_RUNTIME_REQUIRED TRUE )

target_link_libraries(
  atmi_runtime
  -Wl,-as-needed
  ${amd_comgr_LIBRARIES}
  ${LIBELF_LIBRARIES}
  ${ROCM_LIBRARIES}
  -L${ROCM_LIBRARIES_DIR}
  -Wl,--enable-new-dtags
)

if (NOT CMAKE_INSTALL_RPATH)
set_target_properties(atmi_runtime
    PROPERTIES
        SKIP_BUILD_RPATH            FALSE
        BUILD_WITH_INSTALL_RPATH    FALSE
        INSTALL_RPATH_USE_LINK_PATH TRUE
        BUILD_RPATH               "\$ORIGIN;\$ORIGIN/../../hsa/lib;\$ORIGIN/../../lib;\$ORIGIN/../../lib64;\$ORIGIN/../lib64"
        INSTALL_RPATH             "\$ORIGIN;\$ORIGIN/../../hsa/lib;\$ORIGIN/../../lib;\$ORIGIN/../../lib64;\$ORIGIN/../lib64"
  )
endif ()

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
  install(TARGETS atmi_runtime LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT runtime )
endif()

INSTALL(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/atmi.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../include/atmi_runtime.h
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/atmi"
        COMPONENT runtime
       )
INSTALL(FILES
      ${CMAKE_CURRENT_SOURCE_DIR}/../../../bin/cloc.sh
      ${CMAKE_CURRENT_SOURCE_DIR}/../../../bin/mymcpu
      ${CMAKE_CURRENT_SOURCE_DIR}/../../../bin/mygpu
      ${CMAKE_CURRENT_SOURCE_DIR}/../../../bin/gputable.txt
      PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
      DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}/atmi"
      COMPONENT runtime
     )
