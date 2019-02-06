# ===--------------------------------------------------------------------------
#               ATMI (Asynchronous Task and Memory Interface)
#
#  This file is distributed under the MIT License. See LICENSE.txt for details.
# ===--------------------------------------------------------------------------

string( TOLOWER "${ATMI_DEVICE_RUNTIME}" ATMI_DEVICE_RUNTIME_VAR )
if(NOT ATMI_DEVICE_RUNTIME_VAR MATCHES on )
  libatmi_runtime_say("Not building ATMI Device Runtime. Use -DATMI_DEVICE_RUNTIME=on in your cmake options to enable.")
  return()
endif()

#set(amdgcn_mcpus fiji kaveri carrizo hawaii)
#set(amdgcn_mcpus gfx803 gfx700 gfx801 gfx701 gfx800 gfx900 gfx901)
set(amdgcn_mcpus gfx803)

# Target triple
if (NOT DEFINED AMDGPU_TARGET_TRIPLE)
  set(AMDGPU_TARGET_TRIPLE "amdgcn-amd-amdhsa")
endif (NOT DEFINED AMDGPU_TARGET_TRIPLE)

#Find clang
find_package(LLVM QUIET CONFIG PATHS ${LLVM_DIR} NO_DEFAULT_PATH)

if(LLVM_FOUND)
  message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}. Configure: ${LLVM_DIR}/LLVMConfig.cmake")

  #LLVM Compiler info
  set(CLANG_INSTALL_PREFIX ${LLVM_INSTALL_PREFIX})
  set(CLANG_MAIN_INCDIR ${LLVM_BUILD_MAIN_INCLUDE_DIR})

  if (CLANG_INSTALL_PREFIX)
    set(CLANG_BINDIR ${CLANG_INSTALL_PREFIX}/bin)
    set(CLANG_INCDIR ${CLANG_INSTALL_PREFIX}/include)
    set(CLANG_LIBDIR ${CLANG_INSTALL_PREFIX}/lib)
    set(AMDLLVM ${CLANG_INSTALL_PREFIX})
  else()
    set(CLANG_BINDIR ${LLVM_BUILD_BINARY_DIR}/bin)
    set(CLANG_INCDIR ${LLVM_BUILD_BINARY_DIR}/include)
    set(CLANG_LIBDIR ${LLVM_LIBRARY_DIRS})
    set(AMDLLVM ${LLVM_BUILD_BINARY_DIR})
  endif()
else()
    libatmi_runtime_say("Not building ATMI Device Runtime: LLVM not found or path not set using LLVM_DIR.")
    return()
endif()

#Cmake pattern for checking envrionment
set(GFXVAR $ENV{GFXLIST})

#Set mcpus
if (NOT GFXVAR)
  execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../../bin/mygpu
    OUTPUT_VARIABLE MYGPU
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  #message(STATUS "      GPU detected: ${MYGPU}")
  set(mcpus ${MYGPU})
else()
  set(mcpus $ENV{GFXLIST})
endif()
separate_arguments(mcpus)

#device runtime
if(ATMI_WITH_AOMP_VAR)
  execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../../bin/mygpu
    OUTPUT_VARIABLE MYGPU
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  #message(STATUS "      GPU detected: ${MYGPU}")
  find_file (
    ATMI_DE_DEP_LIBHSA_INCLUDE
    ockl_hsa.h
    PATHS
    ${DEVICE_LIB_DIR}/include
    ${CMAKE_INSTALL_PREFIX}/include
    NO_DEFAULT_PATH
    )
  find_file (
    ATMI_DE_DEP_LIBHSA_LIBRARIES
    ockl.amdgcn.bc
    PATHS
    ${DEVICE_LIB_DIR}/lib
    ${CMAKE_INSTALL_PATH}/lib/libdevice/${MYGPU}
    NO_DEFAULT_PATH
    )
else()
  find_file (
    ATMI_DE_DEP_LIBHSA_INCLUDE
    ockl_hsa.h
    PATHS
    ${DEVICE_LIB_DIR}/include
    /opt/rocm/include
    /usr/local/include
    NO_DEFAULT_PATH
    )
  find_file (
    ATMI_DE_DEP_LIBHSA_LIBRARIES
    ockl.amdgcn.bc
    PATHS
    ${DEVICE_LIB_DIR}/lib
    /opt/rocm/lib
    /usr/local/lib
    NO_DEFAULT_PATH
    )
endif()

get_filename_component (ATMI_DE_DEP_LIBHSA_INCLUDE_DIRS ${ATMI_DE_DEP_LIBHSA_INCLUDE} DIRECTORY)
get_filename_component (ATMI_DE_DEP_LIBHSA_LIBRARIES_DIRS ${ATMI_DE_DEP_LIBHSA_LIBRARIES} DIRECTORY)

find_package_handle_standard_args(
  ATMI_DE_DEP_INC
  "One of these files not found: {ockl_hsa.h}"
  ATMI_DE_DEP_LIBHSA_INCLUDE_DIRS)
find_package_handle_standard_args(
  ATMI_DE_DEP_LIB
  "One of these files not found: {ockl.amdgcn.bc}"
  ATMI_DE_DEP_LIBHSA_LIBRARIES_DIRS)

if(ATMI_DE_DEP_LIB_FOUND)
  libatmi_runtime_say("Preparing to build device_runtime with GFX_VER=${GFX_VER}")
else()
  libatmi_runtime_say("Not building ATMI Device Runtime: ROCm Device Libs not found.")
  return()
endif()

# Build
mark_as_advanced(
  ATMI_DE_DEP_LIBHSA_INCLUDE_DIRS
  ATMI_DE_DEP_LIBHSA_LIBRARIES_DIRS)

get_filename_component (ROCM_DEVICE_PATH
  ${ATMI_DE_DEP_LIBHSA_LIBRARIES_DIRS} DIRECTORY
  )

include(bc.cmake)

file(GLOB sources
  ${CMAKE_CURRENT_SOURCE_DIR}/*.ll
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cl
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cu
  )

# set output dir for .h files
set (OUTPUT_INC_DIRECTORY ${ATMI_RUNTIME_PATH}/include)

# create output dir for include files
execute_process(COMMAND "/bin/mkdir" "-p" "${OUTPUT_INC_DIRECTORY}")
add_custom_command(
  OUTPUT ${OUTPUT_INC_DIRECTORY}/atmi_kl.h
  COMMAND /usr/bin/rsync ${CMAKE_CURRENT_SOURCE_DIR}/../../include/atmi_kl.h ${OUTPUT_INC_DIRECTORY}/atmi_kl.h
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../../include/*.h
  )
add_custom_target(device_header ALL DEPENDS ${OUTPUT_INC_DIRECTORY}/atmi_kl.h)

# append to the configure file
file(APPEND ${ATMI_RUNTIME_PATH}/atmi-config.mak "AMDLLVM=${AMDLLVM}\n")
file(APPEND ${ATMI_RUNTIME_PATH}/atmi-config.mak "AMDGPU_TARGET_TRIPLE=${AMDGPU_TARGET_TRIPLE}\n")
file(APPEND ${ATMI_RUNTIME_PATH}/atmi-config.mak "ROCM_DEVICE_PATH=${ROCM_DEVICE_PATH}\n")

# create bc files
foreach(mcpu ${mcpus})
  execute_process(COMMAND /bin/bash "-c" "/bin/echo ${mcpu} | /bin/sed 's/gfx//'"
    OUTPUT_VARIABLE GFXNUM
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  libatmi_runtime_say("build device_runtime with GFXNUM=${GFXNUM}")

  set(optimization_level 2)

  # set output dir for .bc files
  if(ATMI_WITH_AOMP_VAR)
    set(OUTPUTDIR ${ATMI_RUNTIME_PATH}/lib/libdevice)
  else()
    set(OUTPUTDIR ${ATMI_RUNTIME_PATH}/lib)
  endif()

  # create output dir for include files
  execute_process(COMMAND "/bin/mkdir" "-p" "${OUTPUTDIR}")

  add_bc_library(atmi ${CMAKE_CURRENT_SOURCE_DIR} ${sources})

  install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/atmi_kl.h
    DESTINATION include COMPONENT device_runtime)

  if(ATMI_WITH_AOMP_VAR)
    if(CMAKE_BUILD_TYPE MATCHES Debug)
      install(FILES
        ${OUTPUTDIR}/libatmi-${mcpu}.bc
        DESTINATION lib-debug/libdevice COMPONENT device_runtime)
    else()
      install(FILES
        ${OUTPUTDIR}/libatmi-${mcpu}.bc
        DESTINATION lib/libdevice COMPONENT device_runtime)
    endif()
  else()
    if(CMAKE_BUILD_TYPE MATCHES Debug)
      install(FILES
        ${OUTPUTDIR}/atmi-${mcpu}.amdgcn.bc
        DESTINATION lib-debug COMPONENT device_runtime
        )
    else()
      install(FILES
        ${OUTPUTDIR}/atmi-${mcpu}.amdgcn.bc
        DESTINATION lib COMPONENT device_runtime
        )
    endif()
  endif()
endforeach()

