# Copyright (c) 2022 Advanced Micro Devices, Inc. All Rights Reserved.
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

cmake_minimum_required(VERSION 3.16.8)

set(ATMI_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(ATMI_WRAPPER_DIR ${ATMI_BUILD_DIR}/wrapper_dir)
set(ATMI_WRAPPER_INC_DIR ${ATMI_WRAPPER_DIR}/include)
set(ATMI_WRAPPER_BIN_DIR ${ATMI_WRAPPER_DIR}/bin)
if(CMAKE_BUILD_TYPE MATCHES Debug)
  set(ATMI_LIB_DIR "lib-debug")
  set(ROCM_LIB_DIR "lib-debug")
else()
  set(ATMI_LIB_DIR "lib")
  set(ROCM_LIB_DIR "${CMAKE_INSTALL_LIBDIR}")
endif()
set(ATMI_WRAPPER_LIB_DIR ${ATMI_WRAPPER_DIR}/${ATMI_LIB_DIR})

set(PUBLIC_HEADERS
    atmi.h
    atmi_interop_hsa.h
    atmi_runtime.h
  )

#Function to generate header template file
function(create_header_template)
    file(WRITE ${ATMI_WRAPPER_DIR}/header.hpp.in "/*
    Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the \"Software\"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
   */

#ifndef @include_guard@
#define @include_guard@

#ifndef ROCM_HEADER_WRAPPER_WERROR
#define ROCM_HEADER_WRAPPER_WERROR @deprecated_error@
#endif
#if ROCM_HEADER_WRAPPER_WERROR  /* ROCM_HEADER_WRAPPER_WERROR 1 */
#error \"This file is deprecated. Use file from include path /opt/rocm-ver/include/ and prefix with atmi\"
#else     /* ROCM_HEADER_WRAPPER_WERROR 0 */
#if defined(__GNUC__)
#warning \"This file is deprecated. Use file from include path /opt/rocm-ver/include/ and prefix with atmi\"
#else
#pragma message(\"This file is deprecated. Use file from include path /opt/rocm-ver/include/ and prefix with atmi\")
#endif
#endif  /* ROCM_HEADER_WRAPPER_WERROR */

@include_statements@

#endif")
endfunction()

#use header template file and generate wrapper header files
function(generate_wrapper_header)
  file(MAKE_DIRECTORY ${ATMI_WRAPPER_INC_DIR})
  #Generate wrapper header files
  foreach(header_file ${PUBLIC_HEADERS})
    # set include guard
    get_filename_component(INC_GAURD_NAME ${header_file} NAME_WE)
    string(TOUPPER ${INC_GAURD_NAME} INC_GAURD_NAME)
    set(include_guard "${include_guard}ATMI_WRAPPER_INCLUDE_${INC_GAURD_NAME}_H")
    get_filename_component(file_name ${header_file} NAME)
    #set include statements
    set(include_statements "${include_statements}#include \"../../${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/${file_name}\"\n")
    configure_file(${ATMI_WRAPPER_DIR}/header.hpp.in ${ATMI_WRAPPER_INC_DIR}/${file_name})
    unset(include_statements)
  endforeach()

endfunction()

#function to create symlink to binaries
function(create_binary_symlink)
  file(MAKE_DIRECTORY ${ATMI_WRAPPER_BIN_DIR})
  file(GLOB binaries ${CMAKE_CURRENT_SOURCE_DIR}/../bin/*)
  foreach(binary_file ${binaries})
    get_filename_component(file_name ${binary_file} NAME)
    add_custom_target(link_${file_name} ALL
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    COMMAND ${CMAKE_COMMAND} -E create_symlink
                    ../../${CMAKE_INSTALL_LIBEXECDIR}/${PROJECT_NAME}/${file_name} ${ATMI_WRAPPER_BIN_DIR}/${file_name})
  endforeach()
endfunction()

function(create_library_symlink)
  file(MAKE_DIRECTORY ${ATMI_WRAPPER_LIB_DIR})
  set(LIB_ATMI "libatmi_runtime.so")
  #Resuing the code from runtime/core/Cmakelists.txt:
  #This also need to be changed if Major or Minor version changes
  set(LIB_VERSION_MAJOR 0)
  set(LIB_VERSION_MINOR 7)
  if(${ROCM_PATCH_VERSION})
    set(LIB_VERSION_PATCH ${ROCM_PATCH_VERSION})
  else()
    set(LIB_VERSION_PATCH 0)
  endif()
  set(SO_VERSION "${LIB_VERSION_MAJOR}.${LIB_VERSION_MINOR}.${LIB_VERSION_PATCH}")
  set(library_files "${LIB_ATMI}"  "${LIB_ATMI}.${LIB_VERSION_MAJOR}" "${LIB_ATMI}.${SO_VERSION}")
  foreach(file_name ${library_files})
     add_custom_target(link_${file_name} ALL
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                  COMMAND ${CMAKE_COMMAND} -E create_symlink
                  ../../${ROCM_LIB_DIR}/${file_name} ${ATMI_WRAPPER_LIB_DIR}/${file_name})
  endforeach()
endfunction()

#Creater a template for header file
create_header_template()
#Use template header file and generater wrapper header files
generate_wrapper_header()
install(DIRECTORY ${ATMI_WRAPPER_INC_DIR} DESTINATION ${PROJECT_NAME} COMPONENT runtime)
# Create symlink to binaries
create_binary_symlink()
install(DIRECTORY ${ATMI_WRAPPER_BIN_DIR}  DESTINATION ${PROJECT_NAME} COMPONENT runtime)
# Create symlink to libraries
create_library_symlink()
install(DIRECTORY ${ATMI_WRAPPER_LIB_DIR}  DESTINATION ${PROJECT_NAME} COMPONENT runtime)
