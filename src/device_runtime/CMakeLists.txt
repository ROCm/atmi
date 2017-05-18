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

find_package(LLVM REQUIRED CONFIG PATHS ${LLVM_DIR} NO_DEFAULT_PATH)
list(APPEND CMAKE_MODULE_PATH ${LLVM_CMAKE_DIR})
include(AddLLVM)

find_file (
  ATMI_DE_DEP_LIBHSA_INCLUDE
  ockl_hsa.h
  PATHS
  /opt/rocm/include
  /usr/local/include
  )
get_filename_component (ATMI_DE_DEP_LIBHSA_INCLUDE_DIRS ${ATMI_DE_DEP_LIBHSA_INCLUDE} DIRECTORY)

find_file (
  ATMI_DE_DEP_LIBHSA_LIBRARIES
  ockl.amdgcn.bc
  PATHS
  /opt/rocm/lib
  /usr/local/lib
  )
get_filename_component (ATMI_DE_DEP_LIBHSA_LIBRARIES_DIRS ${ATMI_DE_DEP_LIBHSA_LIBRARIES} DIRECTORY)

find_package_handle_standard_args(
  ATMI_DE_DEP_LIBHSA
  "One of these files not found: {ockl_hsa.h, ockl.amdgcn.bc}"
  ATMI_DE_DEP_LIBHSA_LIBRARIES_DIRS
  ATMI_DE_DEP_LIBHSA_INCLUDE_DIRS)

mark_as_advanced(
  ATMI_DE_DEP_LIBHSA_INCLUDE_DIRS
  ATMI_DE_DEP_LIBHSA_LIBRARIES_DIRS)
 
if(ATMI_DE_DEP_LIBHSA_FOUND)
    libatmi_runtime_say("Preparing to build device_runtime with GFX_VER=${GFX_VER}")
    #set (CMAKE_C_COMPILER clang)
    set (CMAKE_C_COMPILER ${LLVM_TOOLS_BINARY_DIR}/clang)

    if(GFX_VER)
    else()
        set(GFX_VER "803")
    endif()


    list(APPEND CMAKE_OCL_SOURCE_FILE_EXTENSIONS .cl)
    list(APPEND CMAKE_C_SOURCE_FILE_EXTENSIONS .cl)
    list(APPEND CMAKE_CXX_SOURCE_FILE_EXTENSIONS .cl)

    set (CLANG_OCL_FLAGS "-Werror -x cl -Xclang -cl-std=CL2.0 -fblocks -target amdgcn--amdhsa -DCL_VERSION_2_0=200 -D__OPENCL_C_VERSION__=200 -Dcl_khr_fp64 -Dcl_khr_fp16 \
    -Dcl_khr_subgroups -Dcl_khr_int64_base_atomics -Dcl_khr_int64_extended_atomics -Xclang -finclude-default-header -target amdgcn--amdhsa -mcpu=gfx${GFX_VER} ${CLANG_OPTIONS_APPEND}")
    set (CLANG_OCL_LINK_FLAGS "-target amdgcn--amdhsa -mcpu=gfx${GFX_VER}")
    
    # FIXME: this is a macro defn file from rocm-device-libs with 
    # no prepare-builtins
    include(OCL.cmake)

    file(GLOB sources
      ${CMAKE_CURRENT_SOURCE_DIR}/*.cl
    )

    # FIXME: add rocm-device-libs as dependency and capture its include dir
    # FIXME: change this to point to rocm-device-libs/include
    include_directories(${ATMI_DE_DEP_LIBHSA_INCLUDE_DIRS}/../../include)
    include_directories(${ATMI_DE_DEP_LIBHSA_INCLUDE_DIRS})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../include)
    
    clang_opencl_bc_builtins_lib(atmi ${CMAKE_CURRENT_SOURCE_DIR} ${sources})
    
    install(FILES 
            ${CMAKE_CURRENT_SOURCE_DIR}/../../include/atmi_kl.h
            DESTINATION include COMPONENT device_runtime)
else()
    libatmi_runtime_say("Not building ATMI Device Runtime: ROCm Device Libs not found.")
endif()
