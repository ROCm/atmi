#
#MIT License 
#
#Copyright © 2016 Advanced Micro Devices, Inc.  
#
#Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software
#without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
#persons to whom the Software is furnished to do so, subject to the following conditions:
#
#The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
#
#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
#PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
#OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

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

option(DEVICELIB_IS_AMDGCN "Use device lib from amdgcn instead of rocm" OFF)

#device runtime
if(DEVICELIB_IS_AMDGCN)
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
    /opt/rocm/libamdgcn/${MYGPU}/include
    NO_DEFAULT_PATH
    )
  find_file (
    ATMI_DE_DEP_LIBHSA_LIBRARIES
    ockl.amdgcn.bc
    PATHS
    ${DEVICE_LIB_DIR}/lib
    /opt/rocm/libamdgcn/${MYGPU}/lib
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
  ${ATMI_DE_DEP_LIBHSA_INCLUDE_DIRS} DIRECTORY
  )

if(DEVICELIB_IS_AMDGCN)
  get_filename_component (ROCM_DEVICE_PATH
    ${ROCM_DEVICE_PATH} DIRECTORY
    )
endif()

get_filename_component (ATMI_RUNTIME_PATH
  ${CMAKE_CURRENT_BINARY_DIR} DIRECTORY
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

# create configure file
file(APPEND ${ATMI_RUNTIME_PATH}/atmi-config.mak "AMDLLVM=${AMDLLVM}\n")
file(APPEND ${ATMI_RUNTIME_PATH}/atmi-config.mak "AMDGPU_TARGET_TRIPLE=${AMDGPU_TARGET_TRIPLE}\n")
file(APPEND ${ATMI_RUNTIME_PATH}/atmi-config.mak "ROCM_DEVICE_PATH=${ROCM_DEVICE_PATH}\n")
file(APPEND ${ATMI_RUNTIME_PATH}/atmi-config.mak "ATMI_RUNTIME_PATH=${ATMI_RUNTIME_PATH}\n")

# create bc files
foreach(mcpu ${mcpus})
  execute_process(COMMAND /bin/bash "-c" "/bin/echo ${mcpu} | /bin/sed 's/gfx//'"
    OUTPUT_VARIABLE GFXNUM
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  libatmi_runtime_say("build device_runtime with GFXNUM=${GFXNUM}")

  set(optimization_level 2)

  # set output dir for .bc files
  if(${ROCM_DEVICE_PATH} MATCHES .*amdgcn.*)
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

  set(HCC2 $ENV{HCC2})
  if(HCC2)
     if(${ROCM_DEVICE_PATH} MATCHES .*amdgcn.*)
       if(CMAKE_BUILD_TYPE MATCHES Debug)
         install(FILES
           ${OUTPUTDIR}/libatmi-${mcpu}.bc
           DESTINATION lib-debug/libdevice/${mcpu} COMPONENT device_runtime
           RENAME libatmi.bc)
       else()
         install(FILES
           ${OUTPUTDIR}/libatmi-${mcpu}.bc
           DESTINATION lib/libdevice/${mcpu} COMPONENT device_runtime
           RENAME libatmi.bc)
       endif()
     else()
       if(CMAKE_BUILD_TYPE MATCHES Debug)
         install(FILES
           ${OUTPUTDIR}/atmi.amdgcn.bc
           DESTINATION lib-debug COMPONENT device_runtime
           )
       else()
         install(FILES
           ${OUTPUTDIR}/atmi.amdgcn.bc
           DESTINATION lib COMPONENT device_runtime
           )
       endif()
     endif()
  else()
     if(${ROCM_DEVICE_PATH} MATCHES .*amdgcn.*)
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
           ${OUTPUTDIR}/atmi.amdgcn.bc
           DESTINATION lib-debug COMPONENT device_runtime
           )
       else()
         install(FILES
           ${OUTPUTDIR}/atmi.amdgcn.bc
           DESTINATION lib COMPONENT device_runtime
           )
       endif()
     endif()
  endif() # end if HCC2
endforeach()

